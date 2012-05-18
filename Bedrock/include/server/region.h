#include "nbt/tag.h"

#include <limits.h>

struct bedrock_region
{
	struct bedrock_world *world;
	int x;
	int z;
	char path[PATH_MAX];
	bedrock_list columns;
};

extern struct bedrock_region *region_create(struct bedrock_world *world, int x, int z);
extern void region_load(struct bedrock_region *region);
extern void region_free(struct bedrock_region *region);
/* Finds the column which contains the point x and z */
extern nbt_tag *find_column_which_contains(struct bedrock_region *region, double x, double z);
