#include "util/yml.h"
#include "util/memory.h"

#define min(x, y) ((x) < (y) ? (x) : (y))

struct yaml_object *yaml_parse(const char *file)
{
	FILE *f;
	yaml_parser_t parser;
	yaml_token_t token;
	int done;
	struct yaml_object *object = NULL;
	char *scalar = NULL;

	f = fopen(file, "rb");
	if (!f)
		return NULL;

	if (!yaml_parser_initialize(&parser))
	{
		fclose(f);
		return NULL;
	}

	yaml_parser_set_input_file(&parser, f);
	do
	{
		if (!yaml_parser_scan(&parser, &token))
			break;

		switch (token.type)
		{
			case YAML_STREAM_START_TOKEN:
				bedrock_assert(object == NULL, ;);
				object = bedrock_malloc(sizeof(struct yaml_object));
				strncpy(object->name, "root", sizeof(object->name));
				break;
			case YAML_STREAM_END_TOKEN:
				bedrock_assert(object->parent == NULL, ;);
				break;
			case YAML_BLOCK_MAPPING_START_TOKEN:
			{
				struct yaml_object *obj = bedrock_malloc(sizeof(struct yaml_object));
				obj->parent = object;
				bedrock_assert(object != NULL, ;);
				bedrock_list_add(&object->objects, obj);

				scalar = obj->name;

				object = obj;
				break;
			}
			case YAML_BLOCK_ENTRY_TOKEN:
			{
				struct yaml_object *obj = bedrock_malloc(sizeof(struct yaml_object));
				obj->parent = object;
				bedrock_assert(object != NULL, ;);
				bedrock_list_add(&object->objects, obj);

				object = obj;
				strncpy(obj->name, "entry", sizeof(obj->name));
				break;
			}
			case YAML_BLOCK_END_TOKEN:
				bedrock_assert(object != NULL, ;);
				if (object->parent != NULL)
					object = object->parent;
				break;
			case YAML_KEY_TOKEN:
				if (scalar == NULL)
				{
					struct yaml_object *obj = bedrock_malloc(sizeof(struct yaml_object));
					obj->parent = object;
					bedrock_assert(object != NULL, ;);
					bedrock_list_add(&object->objects, obj);
					scalar = obj->name;

					object = obj;
				}
				break;
			case YAML_VALUE_TOKEN:
			{
				bedrock_assert(object != NULL, ;);
				bedrock_assert(scalar == NULL, ;);
				scalar = object->value;

				break;
			}
			case YAML_SCALAR_TOKEN:
				bedrock_assert(scalar != NULL, ;);
				bedrock_assert(*scalar == 0, ;);
				memcpy(scalar, token.data.scalar.value, min(token.data.scalar.length, BEDROCK_MAX_STRING_LENGTH));
				if (scalar == object->value)
					object = object->parent;
				scalar = NULL;
				break;
			case YAML_FLOW_SEQUENCE_START_TOKEN:
			case YAML_FLOW_ENTRY_TOKEN:
			{
				struct yaml_object *obj = bedrock_malloc(sizeof(struct yaml_object));
				obj->parent = object;
				bedrock_assert(object != NULL, ;);
				bedrock_list_add(&object->objects, obj);

				scalar = obj->value;
				object = obj;

				strncpy(obj->name, "sequence", sizeof(obj->name));
				break;
			}
			case YAML_FLOW_SEQUENCE_END_TOKEN:
				object = object->parent;
				if (scalar != NULL)
				{
					object->objects.free = (bedrock_free_func) yaml_object_free;
					bedrock_list_clear(&object->objects);

					object = object->parent;
					scalar = NULL;
				}
				break;
			case YAML_TAG_TOKEN:
				strncpy(object->type, (char *) token.data.tag.suffix, sizeof(object->type));
				break;
			case YAML_BLOCK_SEQUENCE_START_TOKEN:
				break;
			default:
				bedrock_log(LEVEL_DEBUG, "yml: Unknown type %d", token.type);
		}

		done = token.type == YAML_STREAM_END_TOKEN;

		yaml_token_delete(&token);
	}
	while (!done);

	yaml_parser_delete(&parser);
	fclose(f);

	return object;
}

void yaml_object_free(struct yaml_object *obj)
{
	obj->objects.free = (bedrock_free_func) yaml_object_free;
	bedrock_list_clear(&obj->objects);

	bedrock_free(obj);
}

static void recursive_yaml_dump(FILE *out, struct yaml_object *obj, int level)
{
	int r;

	for (r = 0; r < level; ++r)
		fprintf(out, "  ");
	
	if (*obj->name)
		fprintf(out, "%s", obj->name);
	else
		fprintf(out, "(null)");
	fprintf(out, " -> ");

	if (*obj->value)
		fprintf(out, "%s (%s)\n", obj->value, obj->type);
	else
	{
		bedrock_node *node;

		fprintf(out, "(%s)\n", obj->type);

		LIST_FOREACH(&obj->objects, node)
		{
			recursive_yaml_dump(out, node->data, level + 1);
		}
	}
}

void yaml_dump(struct yaml_object *obj)
{
	recursive_yaml_dump(stdout, obj, 0);
}

