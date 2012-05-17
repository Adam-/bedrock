#ifndef BEDROCK_SERVER_WORLD_H
#define BEDROCK_SERVER_WORLD_H

#include "util/list.h"
#include "nbt/tag.h"

#include <limits.h>

typedef struct
{
	char name[128];
	char path[PATH_MAX];
	nbt_tag *data;
	bedrock_list regions;
} bedrock_world;

extern bedrock_list world_list;

extern bedrock_world *world_create(const char *name, const char *path);
extern bool world_load(bedrock_world *world);
extern void world_free(bedrock_world *world);
extern bedrock_world *world_find(const char *name);

/* Finds the region which contains the point x and z */
extern bedrock_region *find_region_which_contains(double x, double z);
/* Finds the column which contains the point x and z */
extern nbt_tag *find_column_which_contains(bedrock_world *world, double x, double z);

#endif // BEDROCK_SERVER_WORLD_H
