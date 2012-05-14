#include "nbt/tag.h"
#include "server/world.h"

#include <limits.h>

typedef struct
{
	bedrock_world *world;
	int x;
	int z;
	char path[PATH_MAX];
	bedrock_list columns;
} bedrock_region;

extern bedrock_region *region_create(bedrock_world *world, int x, int z);
extern void region_load(bedrock_region *region);
extern void region_free(bedrock_region *region);
