#include "server/bedrock.h"
#include "blocks/items.h"
#include "blocks/blocks.h"
#include "util/string.h"
#include "util/yml.h"

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
		struct item *i;
		struct yaml_object *yaml;
		bedrock_node *node;

		if (!strstr(ent->d_name, ".yml"))
			continue;

		snprintf(filename, sizeof(filename), "data/items/%s", ent->d_name);
		yaml = yaml_parse(filename);
		if (yaml == NULL)
			continue;

		i = bedrock_malloc(sizeof(struct item));

		LIST_FOREACH(&yaml->objects, node)
		{
			struct yaml_object *attr = node->data;

			if (!strcmp(attr->name, "id"))
				i->id = atoi(attr->value);
			else if (!strcmp(attr->name, "name"))
				i->name = bedrock_strdup(attr->value);
			else if (!strcmp(attr->name, "damageable"))
				i->flags |= ITEM_FLAG_DAMAGABLE;
			else if (!strcmp(attr->name, "enchantable"))
				i->flags |= ITEM_FLAG_ENCHANTABLE;
			else if (!strcmp(attr->name, "tool"))
			{
				if (!strcmp(attr->value, "shovel"))
					i->flags |= ITEM_FLAG_SHOVEL;
				else if (!strcmp(attr->value, "pickaxe"))
					i->flags |= ITEM_FLAG_PICKAXE;
				else if (!strcmp(attr->value, "axe"))
					i->flags |= ITEM_FLAG_AXE;
				else if (!strcmp(attr->value, "sword"))
					i->flags |= ITEM_FLAG_SWORD;
				else if (!strcmp(attr->value, "hoe"))
					i->flags |= ITEM_FLAG_HOE;
				else if (!strcmp(attr->value, "shears"))
					i->flags |= ITEM_FLAG_SHEARS;
			}
			else if (!strcmp(attr->name, "tool-metal"))
			{
				if (!strcmp(attr->value, "gold"))
					i->flags |= ITEM_FLAG_GOLD;
				else if (!strcmp(attr->value, "diamond"))
					i->flags |= ITEM_FLAG_DIAMOND;
				else if (!strcmp(attr->value, "iron"))
					i->flags |= ITEM_FLAG_IRON;
				else if (!strcmp(attr->value, "stone"))
					i->flags |= ITEM_FLAG_STONE;
				else if (!strcmp(attr->value, "wood"))
					i->flags |= ITEM_FLAG_WOOD;
			}
		}

		yaml_object_free(yaml);

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

