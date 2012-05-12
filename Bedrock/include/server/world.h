#include "util/list.h"

#include <limits.h>

typedef struct
{
	char name[128];
	char path[PATH_MAX];
	bedrock_list regions;
} bedrock_world;

extern bedrock_list world_list;

extern bedrock_world *bedrock_world_create(const char *name, const char *path);
extern void bedrock_world_free(bedrock_world *world);
