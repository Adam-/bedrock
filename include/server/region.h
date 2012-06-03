#include "nbt/tag.h"
#include "util/thread.h"

#include <limits.h>

struct bedrock_region
{
	struct bedrock_world *world;
	int x;
	int z;
	char path[PATH_MAX];

	/* Must be held when reading or writing to columns! */
	bedrock_mutex column_mutex;
	/* Columns in this region */
	bedrock_list columns2;

	/* The number of columns in this region in use by players. +1 per player per column.
	 * When this reaches 0 no players are in nor in render distance of this region, and
	 * the region can be unloaded from memory.
	 */
	unsigned int player_column_count;
};

extern bedrock_memory_pool region_pool;

extern struct bedrock_region *region_create(struct bedrock_world *world, int x, int z);
extern void region_free(struct bedrock_region *region);
extern void region_queue_free(struct bedrock_region *region);
extern void region_free_queue();
/* Finds the column which contains the point x and z */
extern struct bedrock_column *find_column_which_contains(struct bedrock_region *region, double x, double z);
