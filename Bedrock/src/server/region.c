#include "server/bedrock.h"
#include "server/region.h"
#include "util/endian.h"
#include "nbt/nbt.h"
#include "compression/compression.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

#define REGION_HEADER_SIZE 1024
#define REGION_SECTOR_SIZE 4096

bedrock_region *bedrock_region_create(const char *path, int x, int z)
{
	bedrock_region *region = bedrock_malloc(sizeof(bedrock_region));
	region->x = x;
	region->z = z;
	strncpy(region->path, path, sizeof(region->path));
	region->columns.free = nbt_free;
	return region;
}

void bedrock_region_load(bedrock_region *region)
{
	int i;
	struct stat file_info;
	unsigned char *file_base;

	bedrock_assert(region->columns.count == 0);

	i = open(region->path, O_RDONLY);
	if (i == -1)
	{
		bedrock_log(LEVEL_WARN, "region: Unable to load region file %s - %s", region->path, strerror(errno));
		return;
	}

	if (fstat(i, &file_info) != 0)
	{
		bedrock_log(LEVEL_WARN, "region: Unable to stat region file %s - %s", region->path, strerror(errno));
		close(i);
		return;
	}

	file_base = mmap(NULL, file_info.st_size, PROT_READ, MAP_SHARED, i, 0);
	if (file_base == MAP_FAILED)
	{
		bedrock_log(LEVEL_WARN, "region: Unable to map region file %s - %s", region->path, strerror(errno));
		close(i);
		return;
	}

	close(i);

	/* Header appears to consist of REGION_HEADER_SIZE unsigned big endian integers */
	for (i = 0; i < REGION_HEADER_SIZE; ++i)
	{
		unsigned char *f = file_base + (i * sizeof(uint32_t)), *f_offset;
		uint32_t offset;
		uint32_t length;
		compression_buffer *cb;
		nbt_tag *tag;

		memcpy(&offset, f, sizeof(offset));
		convert_from_big_endian(&offset, sizeof(offset));

		if (offset == 0)
			continue;

		/* The bottom byte contains how many sectors the chunk uses.
		 * The other 3 bytes contain the offset to the chunk-sector.
		 * - http://wiki.vg/User:Sprenger120
		 */
		offset >>= 8;

		/* When you've got the offset from the header, you have to move the file pointer to offset * 4096.
		 * After that, you're at the chunk data. It has a tiny header of 5 bytes. The length (4 byte big-endian
		 * integer) and a byte, which shows you the kind of data compression.
		 * - http://wiki.vg/User:Sprenger120
		 */
		f_offset = file_base + (offset * REGION_SECTOR_SIZE);

		memcpy(&length, f_offset, sizeof(length));
		convert_from_big_endian(&length, sizeof(length));
		f_offset += sizeof(length);

		f_offset++;

		/* At this point we're at the beginning of the compressed NBT structure */
		cb = compression_decompress(f_offset, length);
		if (cb == NULL)
		{
			bedrock_log(LEVEL_CRIT, "region: Unable to inflate region at offset %d in %s", offset, region->path);
			continue;
		}

		tag = nbt_parse(cb->data, cb->length);
		compression_free_buffer(cb);
		if (tag == NULL)
		{
			bedrock_log(LEVEL_CRIT, "region: Unable to NBT parse region at offset %d in %s", offset, region->path);
			continue;
		}

		bedrock_list_add(&region->columns, tag);
		bedrock_log(LEVEL_DEBUG, "region: Successfully loaded region at offset %d in %s", offset, region->path);
	}

	munmap(file_base, file_info.st_size);
}

void bedrock_region_free(bedrock_region *region)
{
	bedrock_list_clear(&region->columns);
	bedrock_free(region);
}
