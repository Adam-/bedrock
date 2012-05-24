#include "server/bedrock.h"
#include "server/region.h"
#include "util/endian.h"
#include "nbt/nbt.h"
#include "compression/compression.h"
#include "util/memory.h"
#include "server/world.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#define REGION_HEADER_SIZE 1024
#define REGION_SECTOR_SIZE 4096

#define REGION_BUFFER_SIZE 65536

struct bedrock_region *region_create(struct bedrock_world *world, int x, int z)
{
	struct bedrock_region *region = bedrock_malloc(sizeof(struct bedrock_region));
	region->world = world;
	region->x = x;
	region->z = z;
	snprintf(region->path, sizeof(region->path), "%s/region/r.%d.%d.mca", world->path, x, z);
	region->columns.free = (bedrock_free_func) nbt_free;
	bedrock_list_add(&world->regions, region);
	return region;
}

void region_load(struct bedrock_region *region)
{
	int i;
	struct stat file_info;
	char *file_base;
	compression_buffer *cb;

	bedrock_assert(region->columns.count == 0, return);

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

	cb = compression_decompress_init(REGION_BUFFER_SIZE);

	/* Header appears to consist of REGION_HEADER_SIZE unsigned big endian integers */
	for (i = 0; i < REGION_HEADER_SIZE; ++i)
	{
		char *f = file_base + (i * sizeof(uint32_t)), *f_offset;
		uint32_t offset;
		uint32_t length;
		int32_t *x, *z;
		nbt_tag *tag;

		memcpy(&offset, f, sizeof(offset));
		convert_endianness((unsigned char *) &offset, sizeof(offset));

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
		convert_endianness((unsigned char *) &length, sizeof(length));
		f_offset += sizeof(length);

		bedrock_assert(*f_offset++ == 2, continue);

		/* At this point we're at the beginning of the compressed NBT structure */
		compression_decompress_inflate(cb, f_offset, length);
		if (cb == NULL)
		{
			bedrock_log(LEVEL_CRIT, "region: Unable to inflate column at offset %d in %s", offset, region->path);
			continue;
		}

		tag = nbt_parse(cb->buffer->data, cb->buffer->length);
		compression_decompress_reset(cb);
		if (tag == NULL)
		{
			bedrock_log(LEVEL_CRIT, "region: Unable to NBT parse column at offset %d in %s", offset, region->path);
			continue;
		}

		bedrock_list_add(&region->columns, tag);

		x = nbt_read(tag, TAG_INT, 2, "Level", "xPos");
		z = nbt_read(tag, TAG_INT, 2, "Level", "zPos");
		bedrock_log(LEVEL_DEBUG, "region: Successfully loaded column at %d, %d from %s", *x, *z, region->path);
	}

	compression_decompress_end(cb);

	munmap(file_base, file_info.st_size);
}

void region_free(struct bedrock_region *region)
{
	bedrock_list_del(&region->world->regions, region);
	bedrock_list_clear(&region->columns);
	bedrock_free(region);
}

nbt_tag *find_column_which_contains(struct bedrock_region *region, double x, double z)
{
	bedrock_node *n;
	double column_x = x / BEDROCK_BLOCKS_PER_CHUNK, column_z = z / BEDROCK_BLOCKS_PER_CHUNK;

	column_x = floor(column_x);
	column_z = floor(column_z);

	LIST_FOREACH(&region->columns, n)
	{
		nbt_tag *tag = n->data;

		int32_t *x = nbt_read(tag, TAG_INT, 2, "Level", "xPos"),
				*z = nbt_read(tag, TAG_INT, 2, "Level", "zPos");

		if (*x == column_x && *z == column_z)
			return tag;
		else if (*z > column_z || (*z == column_z && *x > column_x))
			break;
	}

	return NULL;
}
