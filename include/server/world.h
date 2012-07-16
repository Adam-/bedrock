#ifndef BEDROCK_SERVER_WORLD_H
#define BEDROCK_SERVER_WORLD_H

#include "util/list.h"
#include "nbt/tag.h"

#include <linux/limits.h>

struct bedrock_world
{
	char name[128];
	char path[PATH_MAX];
	nbt_tag *data;
	bedrock_list regions;

	int64_t time;
};

extern bedrock_list world_list;

extern struct bedrock_world *world_create(const char *name, const char *path);
extern bool world_load(struct bedrock_world *world);
extern void world_free(struct bedrock_world *world);
extern struct bedrock_world *world_find(const char *name);

#endif // BEDROCK_SERVER_WORLD_H
