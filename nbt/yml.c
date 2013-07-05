#include "nbt/nbt.h"

static void nbt_recursive_parse_yml(nbt_tag *nbt, struct yaml_object *object)
{
	bedrock_node *node;

	LIST_FOREACH(&object->objects, node)
	{
		struct yaml_object *obj = node->data;

		if (!strcmp(obj->type, "byte"))
		{
			int8_t b = atoi(obj->value);
			nbt_add(nbt, TAG_BYTE, obj->name, &b, sizeof(b));
		}
		else if (!strcmp(obj->type, "short"))
		{
			int16_t s = atoi(obj->value);
			nbt_add(nbt, TAG_SHORT, obj->name, &s, sizeof(s));
		}
		else if (!strcmp(obj->type, "int"))
		{
			int32_t i = atoi(obj->value);
			nbt_add(nbt, TAG_INT, obj->name, &i, sizeof(i));
		}
		else if (!strcmp(obj->type, "long"))
		{
			int64_t l = atoi(obj->value);
			nbt_add(nbt, TAG_LONG, obj->name, &l, sizeof(l));
		}
		else if (!strcmp(obj->type, "float"))
		{
			float f = atof(obj->value);
			nbt_add(nbt, TAG_FLOAT, obj->name, &f, sizeof(f));
		}
		else if (!strcmp(obj->type, "double"))
		{
			double d = atof(obj->value);
			nbt_add(nbt, TAG_DOUBLE, obj->name, &d, sizeof(d));
		}
		else if (!strcmp(obj->type, "byte-array"))
		{
			bedrock_assert(false, ;);
		}
		else if (!strcmp(obj->type, "str"))
		{
			nbt_add(nbt, TAG_STRING, obj->name, obj->value, strlen(obj->value));
		}
		else if (!strcmp(obj->type, "list"))
		{
			nbt_tag *list = nbt_add(nbt, TAG_LIST, obj->name, NULL, 0);
			nbt_recursive_parse_yml(list, obj);
		}
		else if (!strcmp(obj->type, "compound"))
		{
			nbt_tag *compound = nbt_add(nbt, TAG_COMPOUND, obj->name, NULL, 0);
			nbt_recursive_parse_yml(compound, obj);
		}
		else if (!strcmp(obj->type, "int-array"))
		{
			bedrock_assert(false, ;);
		}
		else
		{
			bedrock_log(LEVEL_WARN, "nbt: yml: Unkown nbt type: %s", obj->type);
			bedrock_assert(false, ;);
		}
	}
}

nbt_tag *nbt_parse_yml(struct yaml_object *obj)
{
	nbt_tag *nbt = nbt_add(NULL, TAG_COMPOUND, obj->name, NULL, 0);
	nbt_recursive_parse_yml(nbt, obj);
	return nbt;
}

