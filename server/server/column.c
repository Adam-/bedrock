#include "server/column.h"
#include "server/client.h"
#include "entities/entity.h"
#include "nbt/nbt.h"
#include "util/compression.h"
#include "util/endian.h"
#include "util/memory.h"
#include "packet/packet_spawn_object.h"
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
	struct column *column;
	bedrock_buffer *nbt_out;
};

void column_load(struct column *column, nbt_tag *data)
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
		bedrock_assert(y < sizeof(column->chunks) / sizeof(struct chunk *), continue);

		bedrock_assert(column->chunks[y] == NULL, continue);

		chunk_load(column, y, chunk_tag);
	}

	tag = nbt_get(data, TAG_BYTE_ARRAY, 2, "Level", "Biomes");
	byte_array = &tag->payload.tag_byte_array;
	column->biomes = (uint8_t *) byte_array->data;
	
	entity_load(column, data);
}

void column_free(struct column *column)
{
	int i;
	bedrock_node *node;

	bedrock_assert(column->players.count == 0, ;);
	bedrock_assert(!column->flags, ;);

	bedrock_log(LEVEL_COLUMN, "column: Freeing column %d,%d", column->x, column->z);

	LIST_FOREACH(&pending_updates, node)
	{
		struct pending_column_update *pc = node->data;
		if (pc->column == column)
		{
			bedrock_list_del_node(&pending_updates, node);
			bedrock_free(node);
			break;
		}
	}
	bedrock_list_del(&column->region->columns, column);

	column->items.free = (bedrock_free_func) column_free_dropped_item;
	bedrock_list_clear(&column->items);

	column->tile_entities.free = (bedrock_free_func) entity_free;
	bedrock_list_clear(&column->tile_entities);

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		chunk_free(column->chunks[i]);

	if (column->data != NULL)
		nbt_free(column->data);

	/* If this is the last column in a region, mark the region as want free.
	 * Because this is the last column it guarentees there are no pending
	 * operations on this region. However the region worker thread may be what
	 * is erasing this column (which can be for a number of reasons), so it isn't
	 * safe to attempt to delete the region here.
	 *
	 * Also be sure the region worker still exists. If it doesn't then the region
	 * is being free'd right now and this column is being free'd as a result of it.
	 */
	if (column->region->columns.count - column->region->empty_columns == 0 && column->region->worker != NULL)
	{
		region_set_pending(column->region, REGION_FLAG_EMPTY);
	}

	bedrock_free(column);
}

uint8_t *column_get_block(struct column *column, int32_t x, uint8_t y, int32_t z)
{
	uint8_t chunk = y / BEDROCK_BLOCKS_PER_CHUNK;
	double column_x = (double) x / BEDROCK_BLOCKS_PER_CHUNK, column_z = (double) z / BEDROCK_BLOCKS_PER_CHUNK;
	struct chunk *c;

	column_x = floor(column_x);
	column_z = floor(column_z);

	bedrock_assert(chunk < BEDROCK_CHUNKS_PER_COLUMN, return NULL);
	bedrock_assert(column_x == column->x && column_z == column->z, return NULL);

	c = column->chunks[chunk];

	if (c == NULL)
		return NULL;

	return chunk_get_block(c, x, y, z);
}

int32_t *column_get_height_for(struct column *column, int32_t x, int32_t z)
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

struct column *find_column_which_contains(struct region *region, double x, double z)
{
	bedrock_node *n;
	double column_x = x / BEDROCK_BLOCKS_PER_CHUNK, column_z = z / BEDROCK_BLOCKS_PER_CHUNK;
	int region_x, region_z;
	struct column *column = NULL;

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
		struct column *c = n->data;

		if (c->x == column_x && c->z == column_z)
		{
			column = c;
			break;
		}
	}

	if (column == NULL)
	{
		struct region_operation *op = bedrock_malloc(sizeof(struct region_operation));
		column = bedrock_malloc(sizeof(struct column));
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
	else if (column->data == NULL)
		column = NULL;

	return column;
}

struct column *find_column_from_world_which_contains(struct world *world, double x, double z)
{
	return find_column_which_contains(find_region_which_contains(world, x, z), x, z);
}

void column_set_pending(struct column *column, enum column_flag flag)
{
	struct pending_column_update *pc;

	if (column->flags)
	{
		column->flags |= flag;
		return;
	}

	pc = bedrock_malloc(sizeof(struct pending_column_update));

	pc->column = column;
	column->flags = flag;

	pending_updates.free = bedrock_free;
	bedrock_list_add(&pending_updates, pc);
}

static bedrock_buffer *column_save_nbt(struct column *column)
{
	struct bedrock_buffer *buffer;

	entity_save(column);

	buffer = nbt_write(column->data);

	entity_cleanup(column);

	return buffer;
}

void column_process_pending()
{
	bedrock_node *node, *node2;

	LIST_FOREACH_SAFE(&pending_updates, node, node2)
	{
		struct pending_column_update *dc = node->data;
		struct column *column = dc->column;

		if (column->flags & COLUMN_FLAG_DIRTY)
		{
			if (column->flags & COLUMN_FLAG_WRITE)
			{
				bedrock_log(LEVEL_WARN, "column: Column %d,%d queued for save but a save is already in progress", column->x, column->z);
			}
			else
			{
				struct region_operation *op = bedrock_malloc(sizeof(struct region_operation));

				op->region = column->region;
				op->operation = REGION_OP_WRITE;
				op->column = column;

				bedrock_log(LEVEL_COLUMN, "column: Starting save for column %d,%d", column->x, column->z);

				op->nbt_out = column_save_nbt(column);

				column->flags &= ~COLUMN_FLAG_DIRTY;
				column->flags |= COLUMN_FLAG_WRITE;

				region_operation_schedule(op);
			}
		}

		/* If there is a flag other than empty set keep the column in the pending state */
		if (!column->flags || column->flags == COLUMN_FLAG_EMPTY)
		{
			/* remove from list */
			bedrock_list_del_node(&pending_updates, node);
			bedrock_free(node);

			/* Column must be *only* empty */
			if (column->flags == COLUMN_FLAG_EMPTY)
			{
				column->flags = 0;
				if (!column->players.count)
					column_free(column);
			}
		}
	}
}

void column_add_item(struct column *column, struct dropped_item *di)
{
	bedrock_node *node;

	bedrock_log(LEVEL_DEBUG, "column: Creating dropped item %s (%d) at %f,%f,%f", di->item->name, di->count, di->x, di->y, di->z);

	bedrock_assert(di->eid == 0, ;);
	di->eid = ++entity_id;
	di->column = column;

	bedrock_list_add(&column->items, di);

	/* Send out this item to nearby players */
	LIST_FOREACH(&column->players, node)
	{
		struct client *client = node->data;

		packet_spawn_object_item(client, di);
	}
}

void column_free_dropped_item(struct dropped_item *item)
{
	bedrock_node *node;

	bedrock_log(LEVEL_DEBUG, "column: Freeing dropped item %s at %f,%f,%f", item->item->name, item->x, item->y, item->z);

	/* Delete this item from nearby players */
	LIST_FOREACH(&item->column->players, node)
	{
		struct client *client = node->data;

		packet_send_destroy_entity_dropped_item(client, item);
	}

	bedrock_list_del(&item->column->items, item);
	bedrock_free(item);
}

struct tile_entity *column_find_tile_entity(struct column *column, int item, int32_t x, uint8_t y, int32_t z)
{
	bedrock_node *node;

	LIST_FOREACH(&column->tile_entities, node)
	{
		struct tile_entity *entity = node->data;

		if (entity->x == x && entity->y == y && entity->z == z)
			if (item == ITEM_NONE || item == entity->blockid)
				return entity;
	}

	return NULL;
}

