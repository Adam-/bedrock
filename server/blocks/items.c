#include "server/bedrock.h"
#include "blocks/items.h"
#include "blocks/blocks.h"
#include "util/string.h"

#include <yaml.h>

#include <sys/types.h>
#include <dirent.h>

bedrock_list items = LIST_INIT;

static void item_free(struct item *i)
{
	bedrock_free(i->name);
	bedrock_free(i);
}

int item_init()
{
	DIR *dir = opendir("data/items");
	struct dirent *ent;

	items.free = (bedrock_free_func) item_free;

	if (!dir)
		return -1;
	
	while ((ent = readdir(dir)))
	{
		char filename[PATH_MAX];
		FILE *f;
		struct item *i;

		yaml_parser_t parser;
		yaml_token_t token;
		yaml_token_type_t type = YAML_NO_TOKEN;
		char *key = NULL;
		int done;

		if (!strstr(ent->d_name, ".yml"))
			continue;

		snprintf(filename, sizeof(filename), "data/items/%s", ent->d_name);
		f = fopen(filename, "rb");
		if (!f)
			continue;

		if (!yaml_parser_initialize(&parser))
			continue;

		i = bedrock_malloc(sizeof(struct item));

		yaml_parser_set_input_file(&parser, f);
		do
		{
			if (!yaml_parser_scan(&parser, &token))
				break;

			if (token.type == YAML_SCALAR_TOKEN)
			{
				if (type == YAML_KEY_TOKEN)
				{
					bedrock_free(key);
					key = bedrock_malloc(token.data.scalar.length);
					memcpy(key, token.data.scalar.value, token.data.scalar.length);
				}
				else if (type == YAML_VALUE_TOKEN)
				{
					const char *value = (const char *) token.data.scalar.value;

					if (!strcmp(key, "id"))
						i->id = atoi(value);
					else if (!strcmp(key, "name"))
						i->name = bedrock_strdup(value);
					else if (!strcmp(key, "damageable"))
						i->flags |= ITEM_FLAG_DAMAGABLE;
					else if (!strcmp(key, "enchantable"))
						i->flags |= ITEM_FLAG_ENCHANTABLE;
					else if (!strcmp(key, "tool"))
					{
						if (!strcmp(value, "shovel"))
							i->flags |= ITEM_FLAG_SHOVEL;
						else if (!strcmp(value, "pickaxe"))
							i->flags |= ITEM_FLAG_PICKAXE;
						else if (!strcmp(value, "axe"))
							i->flags |= ITEM_FLAG_AXE;
						else if (!strcmp(value, "sword"))
							i->flags |= ITEM_FLAG_SWORD;
						else if (!strcmp(value, "hoe"))
							i->flags |= ITEM_FLAG_HOE;
						else if (!strcmp(value, "shears"))
							i->flags |= ITEM_FLAG_SHEARS;
					}
					else if (!strcmp(key, "tool-metal"))
					{
						if (!strcmp(value, "gold"))
							i->flags |= ITEM_FLAG_GOLD;
						else if (!strcmp(value, "diamond"))
							i->flags |= ITEM_FLAG_DIAMOND;
						else if (!strcmp(value, "iron"))
							i->flags |= ITEM_FLAG_IRON;
						else if (!strcmp(value, "stone"))
							i->flags |= ITEM_FLAG_STONE;
						else if (!strcmp(value, "wood"))
							i->flags |= ITEM_FLAG_WOOD;
					}
				}
			}

			done = token.type == YAML_STREAM_END_TOKEN;
			type = token.type;

			yaml_token_delete(&token);
		}
		while (!done);
		bedrock_free(key);
		yaml_parser_delete(&parser);

		fclose(f);

		bedrock_list_add(&items, i);
	}
	
	closedir(dir);

	return 0;
}

void item_shutdown()
{
	bedrock_list_clear(&items);
}

struct item *item_find(enum item_type id)
{
	static struct item i;
	struct block *block;
	bedrock_node *node;

	LIST_FOREACH(&items, node)
	{
		struct item *item = node->data;

		if (item->id == id)
			return item;
	}

	block = block_find(id);

	if (block != NULL)
	{
		i.id = id;
		i.name = block->name;
		i.flags = ITEM_FLAG_BLOCK;
		return &i;
	}

	return NULL;
}

struct item *item_find_by_name(const char *name)
{
	struct block *block;
	bedrock_node *node;

	LIST_FOREACH(&items, node)
	{
		struct item *item = node->data;

		if (!strcmp(name, item->name))
			return item;
	}

	block = block_find_by_name(name);
	if (block != NULL)
		return item_find_or_create(block->id);

	return NULL;
}

struct item *item_find_or_create(enum item_type id)
{
	static struct item i;
	struct item *item = item_find(id);

	if (item == NULL)
	{
		bedrock_log(LEVEL_DEBUG, "item: Unrecognized item %d", id);

		i.id = id;
		i.name = "Unknown";
		i.flags = 0;

		item = &i;
	}

	return item;
}

