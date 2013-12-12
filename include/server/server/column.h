#ifndef BEDROCK_SERVER_COLUMN_H
#define BEDROCK_SERVER_COLUMN_H

#include "server/region.h"
#include "server/chunk.h"
#include "config/hard.h"
#include "blocks/items.h"

enum column_flag
{
	/* Column is dirty and will be written soon */
	COLUMN_FLAG_DIRTY = 1 << 0,
	/* Not in render distance of anyone and will be deleted soon */
	COLUMN_FLAG_EMPTY = 1 << 1,
	/* Column is being loaded from disk */
	COLUMN_FLAG_READ = 1 << 2,
	/* Column is being written to disk */
	COLUMN_FLAG_WRITE = 1 << 3
};

struct column
{
	struct region *region;

	unsigned int flags:4;

	int32_t x;
	int32_t z;

	int idx;

	/* Players in render distance of this column */
	bedrock_list players;

	/* Data for this column. Note that the 'Sections' section is NOT here. We store it below. */
	nbt_tag *data;

	/* Chunks in this column */
	struct chunk *chunks[BEDROCK_CHUNKS_PER_COLUMN];

	/* Biome data, a pointer to the NBT structure */
	uint8_t *biomes;

	/* List of dropped_item structures for dropped items within the column */
	bedrock_list items;

	/* tile entities in this column */
	bedrock_list tile_entities;
};

/* An item dropped on the map somewhere */
struct dropped_item
{
	/* Entity id */
	uint32_t eid;
	/* Column this item is in */
	struct column *column;
	/* This item */
	struct item *item;
	uint8_t count;
	uint16_t data;
	double x;
	double y;
	double z;
};

extern void column_load(struct column *region, nbt_tag *data);
extern void column_free(struct column *column);
extern uint8_t *column_get_block(struct column *column, int32_t x, uint8_t y, int32_t z);
extern uint8_t column_get_data(struct column *column, int32_t x, uint8_t y, int32_t z);
extern int32_t *column_get_height_for(struct column *column, int32_t x, int32_t z);
/* Finds the column which contains the point x and z */
extern struct column *find_column_which_contains(struct region *region, double x, double z);
extern struct column *find_column_from_world_which_contains(struct world *world, double x, double z);

extern void column_set_pending(struct column *column, enum column_flag flag);
extern void column_process_pending();

/* Place a dropped item in the column */
extern void column_add_item(struct column *column, struct dropped_item *di);
extern void column_free_dropped_item(struct dropped_item *column);

extern struct tile_entity *column_find_tile_entity(struct column *column, int item, int32_t x, uint8_t y, int32_t z);

#endif // BEDROCK_SERVER_COLUMN_H
