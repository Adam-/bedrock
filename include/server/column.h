#include "server/region.h"
#include "server/chunk.h"
#include "server/config.h"

struct bedrock_column
{
	struct bedrock_region *region;
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

extern struct bedrock_memory_pool column_pool;

extern struct bedrock_column *column_create(struct bedrock_region *region, nbt_tag *data);
extern void column_free(struct bedrock_column *column);
/* Finds the column which contains the point x and z */
extern struct bedrock_column *find_column_which_contains(struct bedrock_region *region, double x, double z);

/* Allocate a dropped item and place it in this column */
extern struct bedrock_dropped_item *column_create_dropped_item(struct bedrock_column *column, struct bedrock_item *item);
extern void column_free_dropped_item(struct bedrock_dropped_item *column);
