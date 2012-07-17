#include "server/column.h"
#include "server/client.h"
#include "nbt/nbt.h"
#include "util/memory.h"
#include "util/endian.h"
#include "util/timer.h"
#include "compression/compression.h"
#include "packet/packet_spawn_dropped_item.h"
#include "packet/packet_destroy_entity.h"

#include <math.h>

#define DATA_CHUNK_SIZE 2048

/* A list of pending_column_update structures.
 * Columns on this list are either:
 * dirty (modified in memory and need to be written to disk sometime)
 * empty (no players are in render distance of it)
 * or both.
 *
 * Every 6000 ticks we loop this list and if the column is dirty write it,
 * and then if the column is empty delete it.
 */
static bedrock_list pending_updates = LIST_INIT;

struct pending_column_update
{
	struct bedrock_column *column;
	bedrock_buffer *nbt_out;
};

void column_load(struct bedrock_column *column, nbt_tag *data)
{
	int32_t *x, *z;
	nbt_tag *tag;
	bedrock_node *node;
	struct nbt_tag_byte_array *byte_array;

	x = nbt_read(data, TAG_INT, 2, "Level", "xPos");
	z = nbt_read(data, TAG_INT, 2, "Level", "zPos");

	bedrock_assert(*x == column->x, ;);
	bedrock_assert(*z == column->z, ;);

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
		compression_buffer *buffer = compression_compress_init(DATA_CHUNK_SIZE);

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
}

void column_free(struct bedrock_column *column)
{
	int i;

	bedrock_assert(column->players.count == 0, ;);
	bedrock_assert((column->flags & ~COLUMN_FLAG_EMPTY) == 0, ;);

	bedrock_log(LEVEL_DEBUG, "chunk: Freeing column %d,%d", column->x, column->z);

	column->items.free = (bedrock_free_func) column_free_dropped_item;
	bedrock_list_clear(&column->items);

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		chunk_free(column->chunks[i]);

	bedrock_buffer_free(column->biomes);

	if (column->data != NULL)
		nbt_free(column->data);
	bedrock_free(column);
}

uint8_t *column_get_block(struct bedrock_column *column, int32_t x, uint8_t y, int32_t z)
{
	uint8_t chunk = y / BEDROCK_BLOCKS_PER_CHUNK;
	double column_x = (double) x / BEDROCK_BLOCKS_PER_CHUNK, column_z = (double) z / BEDROCK_BLOCKS_PER_CHUNK;
	struct bedrock_chunk *c;

	column_x = floor(column_x);
	column_z = floor(column_z);

	bedrock_assert(chunk < BEDROCK_CHUNKS_PER_COLUMN, return NULL);
	bedrock_assert(column_x == column->x && column_z == column->z, return NULL);

	c = column->chunks[chunk];

	if (c == NULL)
		return NULL;

	chunk_decompress(c);
	return chunk_get_block(c, x, y, z);
}

int32_t *column_get_height_for(struct bedrock_column *column, int32_t x, int32_t z)
{
	struct nbt_tag_int_array *tia = &nbt_get(column->data, TAG_INT_ARRAY, 2, "Level", "HeightMap")->payload.tag_int_array;
	double column_x = (double) x / BEDROCK_BLOCKS_PER_CHUNK, column_z = (double) z / BEDROCK_BLOCKS_PER_CHUNK;
	uint8_t offset;

	column_x = floor(column_x);
	column_z = floor(column_z);

	bedrock_assert(tia->length == BEDROCK_HEIGHTMAP_LENGTH, return NULL);
	bedrock_assert(column_x == column->x && column_z == column->z, return NULL);

	x %= BEDROCK_BLOCKS_PER_CHUNK;
	z %= BEDROCK_BLOCKS_PER_CHUNK;

	if (x < 0)
		x = BEDROCK_BLOCKS_PER_CHUNK - abs(x);
	if (z < 0)
		z = BEDROCK_BLOCKS_PER_CHUNK - abs(z);

	offset = z + (x * BEDROCK_BLOCKS_PER_CHUNK);

	return &tia->data[offset];
}

struct bedrock_column *find_column_which_contains(struct bedrock_region *region, double x, double z)
{
	bedrock_node *n;
	double column_x = x / BEDROCK_BLOCKS_PER_CHUNK, column_z = z / BEDROCK_BLOCKS_PER_CHUNK;
	int region_x, region_z;
	struct bedrock_column *column = NULL;

	if (region == NULL)
		return NULL;

	column_x = floor(column_x);
	column_z = floor(column_z);

	region_x = floor(column_x / BEDROCK_COLUMNS_PER_REGION);
	region_z = floor(column_z / BEDROCK_COLUMNS_PER_REGION);

	bedrock_assert(region_x == region->x, ;);
	bedrock_assert(region_z == region->z, ;);

	LIST_FOREACH(&region->columns, n)
	{
		struct bedrock_column *c = n->data;

		if (c->x == column_x && c->z == column_z)
		{
			column = c;
			break;
		}
	}

	if (column == NULL)
	{
		struct region_operation *op = bedrock_malloc(sizeof(struct region_operation));
		column = bedrock_malloc(sizeof(struct bedrock_column));
		column->region = region;
		column->x = column_x;
		column->z = column_z;

		bedrock_list_add(&region->columns, column);

		column->flags |= COLUMN_FLAG_READ;

		op->region = region;
		op->operation = REGION_OP_READ;
		op->column = column;
		region_operation_schedule(op);

		column = NULL;
	}
	else if (column->flags & COLUMN_FLAG_READ)
		column = NULL;

	return column;
}

void column_set_pending(struct bedrock_column *column, enum bedrock_column_flag flag)
{
	struct pending_column_update *pc;

	if (column == NULL || column->flags & flag)
		return;

	pc = bedrock_malloc(sizeof(struct pending_column_update));

	pc->column = column;
	column->flags |= flag;

	bedrock_list_add(&pending_updates, pc);
}

void column_process_pending(void __attribute__((__unused__)) *notused)
{
	bedrock_node *node, *node2;

	LIST_FOREACH_SAFE(&pending_updates, node, node2)
	{
		struct pending_column_update *dc = node->data;
		struct bedrock_column *column = dc->column;

		if (column->flags & COLUMN_FLAG_DIRTY)
		{
			if (column->flags & COLUMN_FLAG_WRITE)
			{
				bedrock_log(LEVEL_WARN, "column: Column %d,%d queued for save but a save is already in progress", column->x, column->z);
			}
			else
			{
				struct region_operation *op = bedrock_malloc(sizeof(struct region_operation));
				int i;
				nbt_tag *level, *sections, *biomes;
				compression_buffer *buf;

				op->region = column->region;
				op->operation = REGION_OP_WRITE;
				op->column = column;

				bedrock_log(LEVEL_COLUMN, "column: Starting save for column %d,%d", column->x, column->z);

				level = nbt_get(column->data, TAG_COMPOUND, 1, "Level");
				bedrock_assert(level != NULL, ;);

				sections = nbt_add(level, TAG_LIST, "Sections", NULL, 0);

				for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
				{
					struct bedrock_chunk *chunk = column->chunks[i];
					nbt_tag *chunk_tag;

					if (chunk == NULL)
						continue;

					chunk_decompress(chunk);

					chunk_tag = nbt_add(sections, TAG_COMPOUND, "", NULL, 0);

					nbt_add(chunk_tag, TAG_BYTE_ARRAY, "Data", chunk->data, BEDROCK_DATA_LENGTH);
					nbt_add(chunk_tag, TAG_BYTE_ARRAY, "SkyLight", chunk->skylight, BEDROCK_DATA_LENGTH);
					nbt_add(chunk_tag, TAG_BYTE_ARRAY, "BlockLight", chunk->blocklight, BEDROCK_DATA_LENGTH);
					nbt_add(chunk_tag, TAG_BYTE, "Y", &chunk->y, sizeof(chunk->y));
					nbt_add(chunk_tag, TAG_BYTE_ARRAY, "Blocks", chunk->blocks, BEDROCK_BLOCK_LENGTH);

					chunk_compress(chunk);
				}

				buf = compression_decompress(BEDROCK_BIOME_LENGTH, column->biomes->data, column->biomes->length);

				biomes = nbt_add(level, TAG_BYTE_ARRAY, "Biomes", buf->buffer->data, buf->buffer->length);

				compression_decompress_end(buf);

				op->nbt_out = nbt_write(column->data);

				nbt_free(sections);
				nbt_free(biomes);

				column->flags &= ~COLUMN_FLAG_DIRTY;
				column->flags |= COLUMN_FLAG_WRITE;

				region_operation_schedule(op);
			}
		}
		else if (column->flags & COLUMN_FLAG_EMPTY)
		{
			if (column->players.count == 0)
				bedrock_list_del(&column->region->columns, column); // XXX dumb
				//column_free(column);
			else
				column->flags &= ~COLUMN_FLAG_EMPTY;
		}

		bedrock_list_del_node(&pending_updates, node);
		bedrock_free(dc);
		bedrock_free(node);
	}

	bedrock_timer_schedule(6000, column_process_pending, NULL);
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
