#ifndef BEDROCK_UTIL_YML_H
#define BEDROCK_UTIL_YML_H

#include "util/util.h"
#include "server/config/hard.h"
#include "util/list.h"

#include <yaml.h>

struct yaml_object
{
	char name[BEDROCK_MAX_STRING_LENGTH];
	char type[32];

	char value[BEDROCK_MAX_STRING_LENGTH];
	bedrock_list objects;

	struct yaml_object *parent;
};

extern struct yaml_object *yaml_parse(const char *file);
extern void yaml_object_free(struct yaml_object *obj);
extern void yaml_dump(struct yaml_object *obj);

#endif
