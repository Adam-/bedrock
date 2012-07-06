#include "server/column.h"
#include "server/client.h"
#include "nbt/nbt.h"
#include "util/memory.h"
#include "compression/compression.h"
#include "packet/packet_spawn_dropped_item.h"
#include "packet/packet_destroy_entity.h"

#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define DATA_CHUNK_SIZE 2048

struct bedrock_memory_pool column_pool = BEDROCK_MEMORY_POOL_INIT("column memory pool");
static bedrock_list dirty_columns = LIST_INIT;

struct dirty_column
{
	struct bedrock_column *column;
	char region_path[PATH_MAX];
};

struct bedrock_column *column_create(struct bedrock_region *region, nbt_tag *data)
{
	struct bedrock_column *column = bedrock_malloc_pool(&column_pool, sizeof(struct bedrock_column));
	nbt_tag *tag;
	bedrock_node *node;
	struct nbt_tag_byte_array *byte_array;

	column->region = region;
	nbt_copy(data, TAG_INT, &column->x, sizeof(column->x), 2, "Level", "xPos");
	nbt_copy(data, TAG_INT, &column->z, sizeof(column->z), 2, "Level", "zPos");
	column->data = data;

	tag = nbt_get(data, TAG_LIST, 2, "Level", "Sections");
	LIST_FOREACH(&tag->payload.tag_list.list, node)
	{
		nbt_tag *chunk_tag = node->data;
		uint8_t y;

		nbt_copy(chunk_tag, TAG_BYTE, &y, sizeof(y), 1, "Y");
		bedrock_assert(y < sizeof(column->chunks) / sizeof(struct bedrock_chunk *), continue);

		bedrock_assert(column->chunks[y] == NULL, continue);

		chunk_load(column, y, chunk_tag);
	}
	nbt_free(tag);

	{
		compression_buffer *buffer = compression_compress_init(&column_pool, DATA_CHUNK_SIZE);

		tag = nbt_get(data, TAG_BYTE_ARRAY, 2, "Level", "Biomes");
		byte_array = &tag->payload.tag_byte_array;
		bedrock_assert(byte_array->length == BEDROCK_BIOME_LENGTH, ;);

		compression_compress_deflate_finish(buffer, (const unsigned char *) byte_array->data, byte_array->length);
		bedrock_buffer_resize(buffer->buffer, buffer->buffer->length);

		column->biomes = buffer->buffer;
		buffer->buffer = NULL;

		nbt_free(tag);
		compression_compress_end(buffer);
	}

	tag = nbt_get(data, TAG_INT_ARRAY, 2, "Level", "HeightMap");
	nbt_free(tag);

	return column;
}

void column_free(struct bedrock_column *column)
{
	int i;

	bedrock_assert(column->players.count == 0, ;);

	if (column->saving || column->dirty)
	{
		/* We can't free this now because it's being written to disk *or*
		 * has pending changes needing disk write.
		 * Instead detach it from the region, and it will be deleted
		 * later once the write is complete.
		 */
		column->region = NULL;
		return;
	}

	bedrock_log(LEVEL_DEBUG, "chunk: Freeing column %d,%d", column->x, column->z);

	column->items.free = (bedrock_free_func) column_free_dropped_item;
	bedrock_list_clear(&column->items);

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		chunk_free(column->chunks[i]);

	bedrock_buffer_free(column->biomes);

	nbt_free(column->data);
	bedrock_free_pool(&column_pool, column);
}

struct bedrock_column *find_column_which_contains(struct bedrock_region *region, double x, double z)
{
	bedrock_node *n;
	double column_x = x / BEDROCK_BLOCKS_PER_CHUNK, column_z = z / BEDROCK_BLOCKS_PER_CHUNK;
	struct bedrock_column *column = NULL;

	if (region == NULL)
		return NULL;

	column_x = floor(column_x);
	column_z = floor(column_z);

	bedrock_mutex_lock(&region->column_mutex);

	LIST_FOREACH(&region->columns, n)
	{
		struct bedrock_column *c = n->data;

		if (c->x == column_x && c->z == column_z)
		{
			column = c;
			break;
		}
		else if (c->z > column_z || (c->z == column_z && c->x > column_x))
			break;
	}

	bedrock_mutex_unlock(&region->column_mutex);

	return column;
}

void column_dirty(struct bedrock_column *column)
{
	struct dirty_column *dc;

	if (column == NULL || column->dirty == true)
		return;

	dc = bedrock_malloc(sizeof(struct dirty_column));

	dc->column = column;
	strncpy(dc->region_path, column->region->path, sizeof(dc->region_path));

	bedrock_list_add(&dirty_columns, dc);
}

static void column_save_entry(struct dirty_column *dc)
{
	struct bedrock_column *column = dc->column;
	int i;
	int32_t column_x, column_z;
	int offset;
	nbt_tag *tag;
	bedrock_buffer *buf;
	compression_buffer *cb;

	i = open(dc->region_path, O_RDONLY);
	if (i == -1)
	{
		bedrock_log(LEVEL_WARN, "column: Unable to open region file %s for saving - %s", dc->region_path, strerror(errno));
		return;
	}

	close(i);

	column_x = column->x;
	if (column_x < 0)
		column_x = BEDROCK_COLUMNS_PER_REGION - abs(column_x);

	column_z = column->z;
	if (column_z < 0)
		column_z = BEDROCK_COLUMNS_PER_REGION - abs(column_z);

	offset = column_x + column_z * BEDROCK_COLUMNS_PER_REGION;


	/* Convert NBT structure to buffer */
	bedrock_mutex_lock(&column->data_mutex);
	buf = nbt_write(column->data);
	bedrock_mutex_unlock(&column->data_mutex);

	cb = compression_compress_init(&region_pool, DATA_CHUNK_SIZE);

	/* Compress structure */
	compression_compress_deflate_finish(cb, buf->data, buf->length);

	/* Write.. */

	compression_decompress_end(cb);

	bedrock_buffer_free(buf);
}

static void column_save_exit(struct dirty_column *dc)
{
	struct bedrock_column *column = dc->column;
	nbt_tag *tag;

	column->saving = false;

	bedrock_log(LEVEL_COLUMN, "column: Finished save for column %d,%d", column->x, column->z);

	tag = nbt_get(data, TAG_LIST, 2, "Level", "Sections");
	nbt_free(tag);

	tag = nbt_get(data, TAG_LIST, 2, "Level", "Biomes");
	nbt_free(tag);

	/* This column may need to be deleted now */
	if (column->region == NULL)
		column_free(column);

	bedrock_free(dc);
}

void column_save()
{
	bedrock_node *node, *node2;

	LIST_FOREACH_SAFE(&dirty_columns, node, node2)
	{
		struct dirty_column *dc = node->data;
		struct bedrock_column *column = dc->column;

		if (column->saving)
		{
			bedrock_log(LEVEL_WARN, "column: Column %d,%d queued for save but a save is already in progress", column->x, column->z);
			continue;
		}

		bedrock_log(LEVEL_COLUMN, "column: Starting save for column %d,%d", column->x, column->z);

		{
			int i;
			nbt_tag *level, *tag;

			level = nbt_get(client->data, TAG_LIST, 1, "Level");
			bedrock_assert(level != NULL, ;);

			tag = nbt_add(level, TAG_LIST, "Sections", NULL, 0);

			for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
			{
				struct bedrock_chunk *chunk = column->chunks[i];
				nbt_tag *chunk_tag;

				if (chunk == NULL)
					continue;

				chunk_decompress(chunk);

				chunk_tag = nbt_add(tag, TAG_COMPOUND, NULL, NULL, 0);

				nbt_add(chunk_tag, TAG_BYTE_ARRAY, "Data", chunk->data, BEDROCK_DATA_LENGTH);
				nbt_add(chunk_tag, TAG_BYTE_ARRAY, "SkyLight", chunk->skylight, BEDROCK_DATA_LENGTH);
				nbt_add(chunk_tag, TAG_BYTE_ARRAY, "BlockLight", chunk->blocklight, BEDROCK_DATA_LENGTH);
				nbt_add(chunk_tag, TAG_BYTE, "Y", chunk->y, sizeof(chunk->y));
				nbt_add(chunk_tag, TAG_BYTE_ARRAY, "Blocks", chunk->blocks, BEDROCK_BLOCK_LENGTH);

				chunk_compress(chunk);
			}

			nbt_add(tag, TAG_END, NULL, NULL, 0);

			{
				compression_buffer *buf = compression_decompress(NULL, BEDROCK_BIOME_LENGTH, column->biomes->data, column->biomes->length);

				nbt_add(level, TAG_BYTE_ARRAY, "Biomes", buf->buffer->data, buf->buffer->length);

				compression_free(buf);
			}
		}

		column->dirty = false;
		column->saving = true;

		bedrock_thread_start((bedrock_thread_entry) column_save_entry, (bedrock_thread_exit) column_save_exit, dc);

		bedrock_list_del_node(&dirty_columns, node);
		bedrock_free(node);
	}
}

void column_add_item(struct bedrock_column *column, struct bedrock_dropped_item *di)
{
	bedrock_node *node;

	bedrock_log(LEVEL_DEBUG, "column: Creating dropped item %s in at %f,%f,%f", di->item->name, di->x, di->y, di->z);

	bedrock_assert(di->eid == 0, ;);
	di->eid = ++entity_id;
	di->column = column;

	bedrock_list_add(&column->items, di);

	/* Send out this item to nearby players */
	LIST_FOREACH(&column->players, node)
	{
		struct bedrock_client *client = node->data;

		packet_send_spawn_dropped_item(client, di);
	}
}

void column_free_dropped_item(struct bedrock_dropped_item *item)
{
	bedrock_node *node;

	bedrock_log(LEVEL_DEBUG, "column: Freeing dropped item %s at %f,%f,%f", item->item->name, item->x, item->y, item->z);

	/* Delete this item from nearby players */
	LIST_FOREACH(&item->column->players, node)
	{
		struct bedrock_client *client = node->data;

		packet_send_destroy_entity_dropped_item(client, item);
	}

	bedrock_list_del(&item->column->items, item);
	bedrock_free(item);
}
