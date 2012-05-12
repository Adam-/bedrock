#include "nbt/tag.h"

#include <limits.h>

typedef struct
{
	int x;
	int z;
	char path[PATH_MAX];
	bedrock_list columns;
} bedrock_region;

extern bedrock_region *bedrock_region_create(const char *path, int x, int z);
extern void bedrock_region_load(bedrock_region *region);
extern void bedrock_region_free(bedrock_region *region);
