#include "server/region.h"
#include "server/chunk.h"
#include "config/hard.h"
#include "blocks/items.h"

struct bedrock_column
{
	struct bedrock_region *region;

	/* True if this column is dirty */
	bool dirty;
	/* True if this column is being saved */
	bool saving;

	int32_t x;
	int32_t z;

	/* Players in render distance of this column */
	bedrock_list players;

	/* Data for this column. Note that the 'Sections' section is NOT here. We store it below. */
	nbt_tag *data;

	/* Chunks in this column */
	struct bedrock_chunk *chunks[BEDROCK_CHUNKS_PER_COLUMN];

	/* Compressed biome data */
	bedrock_buffer *biomes;

	/* List of bedrock_dropped_item structures for dropped items within the column */
	bedrock_list items;
};

/* An item dropped on the map somewhere */
struct bedrock_dropped_item
{
	/* Entity id */
	uint32_t eid;
	/* Column this item is in */
	struct bedrock_column *column;
	/* This item */
	struct bedrock_item *item;
	uint8_t count;
	uint16_t data;
	double x;
	double y;
	double z;
};

extern struct bedrock_memory_pool column_pool;

extern struct bedrock_column *column_create(struct bedrock_region *region, nbt_tag *data);
extern void column_free(struct bedrock_column *column);
/* Finds the column which contains the point x and z */
extern struct bedrock_column *find_column_which_contains(struct bedrock_region *region, double x, double z);
/* Mark a column as dirty and add it to the dirty_columns list */
extern void column_dirty(struct bedrock_column *column);
extern void column_save();

/* Place a dropped item in the column */
extern void column_add_item(struct bedrock_column *column, struct bedrock_dropped_item *di);
extern void column_free_dropped_item(struct bedrock_dropped_item *column);
