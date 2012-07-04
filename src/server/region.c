#include "server/bedrock.h"
#include "util/endian.h"
#include "nbt/nbt.h"
#include "compression/compression.h"
#include "util/memory.h"
#include "server/world.h"
#include "server/column.h"
#include "util/timer.h"
#include "server/client.h"

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

static bedrock_list empty_regions;

struct bedrock_memory_pool region_pool = BEDROCK_MEMORY_POOL_INIT("region memory pool");

static void region_load(struct bedrock_region *region)
{
	int i;
	struct stat file_info;
	unsigned char *file_base;
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

	cb = compression_decompress_init(&region_pool, REGION_BUFFER_SIZE);

	/* Header appears to consist of REGION_HEADER_SIZE unsigned big endian integers */
	for (i = 0; i < REGION_HEADER_SIZE; ++i)
	{
		unsigned char *f = file_base + (i * sizeof(uint32_t)), *f_offset;
		uint32_t offset;
		uint32_t length;
		nbt_tag *tag;
		struct bedrock_column *column;

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

		column = column_create(region, tag);

		column->offset = offset;

		bedrock_mutex_lock(&region->column_mutex);
		bedrock_list_add(&region->columns, column);
		bedrock_mutex_unlock(&region->column_mutex);

		bedrock_log(LEVEL_COLUMN, "region: Successfully loaded column at %d, %d at offset %d sectors from %s", column->x, column->z, offset, region->path);
	}

	compression_decompress_end(cb);

	munmap(file_base, file_info.st_size);
}

static void region_exit(struct bedrock_region bedrock_attribute_unused *region)
{
	bedrock_node *node;

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *client = node->data;

		if (client->authenticated >= STATE_BURSTING)
		{
			client_update_chunks(client);
		}
	}
}

struct bedrock_region *region_create(struct bedrock_world *world, int x, int z)
{
	struct bedrock_region *region = bedrock_malloc_pool(&region_pool, sizeof(struct bedrock_region));

	region->world = world;
	region->x = x;
	region->z = z;
	snprintf(region->path, sizeof(region->path), "%s/region/r.%d.%d.mca", world->path, x, z);

	bedrock_mutex_init(&region->column_mutex, "region column mutex");
	region->columns.free = (bedrock_free_func) column_free;

	bedrock_list_add(&world->regions, region);

	bedrock_thread_start((bedrock_thread_entry) region_load, (bedrock_thread_exit) region_exit, region);

	return region;
}

void region_free(struct bedrock_region *region)
{
	bedrock_list_del(&empty_regions, region);

	bedrock_list_del(&region->world->regions, region);

	bedrock_mutex_lock(&region->column_mutex);
	bedrock_list_clear(&region->columns);
	bedrock_mutex_unlock(&region->column_mutex);
	bedrock_mutex_destroy(&region->column_mutex);

	bedrock_free_pool(&region_pool, region);
}

void region_queue_free(struct bedrock_region *region)
{
	if (bedrock_list_has_data(&empty_regions, region) == false)
	{
		bedrock_log(LEVEL_COLUMN, "region: Queuing region %d, %d for free", region->x, region->z);
		bedrock_list_add(&empty_regions, region);
	}
}

void region_free_queue()
{
	bedrock_node *node, *node2;

	LIST_FOREACH_SAFE(&empty_regions, node, node2)
	{
		struct bedrock_region *region = node->data;

		if (region->player_column_count == 0)
		{
			bedrock_log(LEVEL_COLUMN, "region: Freeing region %d, %d", region->x, region->z);
			region_free(region);
		}
	}
	bedrock_list_clear(&empty_regions);

	bedrock_timer_schedule(6000, region_free_queue, NULL);
}

struct bedrock_region *find_region_which_contains(struct bedrock_world *world, double x, double z)
{
	double column_x, column_z;
	double region_x, region_z;
	bedrock_node *n;
	struct bedrock_region *region;

	column_x = x / BEDROCK_BLOCKS_PER_CHUNK, column_z = z / BEDROCK_BLOCKS_PER_CHUNK;
	column_x = floor(column_x);
	column_z = floor(column_z);

	region_x = column_x / BEDROCK_COLUMNS_PER_REGION, region_z = column_z / BEDROCK_COLUMNS_PER_REGION;
	region_x = floor(region_x);
	region_z = floor(region_z);

	LIST_FOREACH(&world->regions, n)
	{
		region = n->data;

		// XXX are these in some order?
		if (region->x == region_x && region->z == region_z)
			return region;
	}

	region = region_create(world, region_x, region_z);

	return region;
}

static void region_save_column_entry(struct bedrock_column *column)
{

}

static void region_save_column_exit(struct bedrock_column *column)
{
	column->saving = false;

	bedrock_log(LEVEL_COLUMN, "region: Finished save for column %d,%d", column->x, column->z);

	/* This column may need to be deleted now */
	if (column->region == NULL)
		column_free(column);
}

void region_save()
{
	bedrock_node *node, *node2;

	LIST_FOREACH_SAFE(&dirty_columns, node, node2)
	{
		struct bedrock_column *column = node->data;

		bedrock_log(LEVEL_COLUMN, "region: Starting save for column %d,%d", column->x, column->z);

		column->saving = true;

		bedrock_thread_start((bedrock_thread_entry) region_save_column_entry, (bedrock_thread_exit) region_save_column_exit, column);

		bedrock_list_del_node(&dirty_columns, node);
		bedrock_free(node);
	}
}
