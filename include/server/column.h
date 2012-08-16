#include "server/region.h"
#include "server/chunk.h"
#include "config/hard.h"
#include "blocks/items.h"

enum bedrock_column_flag
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

struct bedrock_column
{
	struct bedrock_region *region;

	unsigned int flags:4;

	int32_t x;
	int32_t z;

	/* Players in render distance of this column */
	bedrock_list players;

	/* Data for this column. Note that the 'Sections' section is NOT here. We store it below. */
	nbt_tag *data;

	/* Chunks in this column */
	struct bedrock_chunk *chunks[BEDROCK_CHUNKS_PER_COLUMN];

	/* Biome data, a pointer to the NBT structure */
	uint8_t *biomes;

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

extern void column_load(struct bedrock_column *region, nbt_tag *data);
extern void column_free(struct bedrock_column *column);
extern uint8_t *column_get_block(struct bedrock_column *column, int32_t x, uint8_t y, int32_t z);
extern int32_t *column_get_height_for(struct bedrock_column *column, int32_t x, int32_t z);
/* Finds the column which contains the point x and z */
extern struct bedrock_column *find_column_which_contains(struct bedrock_region *region, double x, double z);

extern void column_set_pending(struct bedrock_column *column, enum bedrock_column_flag flag);
extern void column_process_pending();

/* Place a dropped item in the column */
extern void column_add_item(struct bedrock_column *column, struct bedrock_dropped_item *di);
extern void column_free_dropped_item(struct bedrock_dropped_item *column);
