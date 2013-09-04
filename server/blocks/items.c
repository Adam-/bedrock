#include "server/bedrock.h"
#include "blocks/items.h"
#include "blocks/blocks.h"
#include "util/string.h"
#include "util/yml.h"

#include <sys/types.h>
#include <dirent.h>

static bedrock_list items = LIST_INIT;
static bedrock_mutex item_mutex;

static void item_free(struct item *i)
{
	bedrock_free(i->name);
	bedrock_free(i);
}

int item_init()
{
	DIR *dir = opendir("data/items");
	struct dirent *ent;

	bedrock_mutex_init(&item_mutex, "item mutex");

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
			else if (!strcmp(attr->name, "furnace_burn_time"))
				i->furnace_burn_time = atoi(attr->value);
			else if (!strcmp(attr->name, "furnace_output"))
				i->furnace_output = atoi(attr->value);
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
	struct block *block;
	bedrock_node *node;

	bedrock_mutex_lock(&item_mutex);
	LIST_FOREACH(&items, node)
	{
		struct item *item = node->data;

		if (item->id == id)
		{
			bedrock_mutex_unlock(&item_mutex);
			return item;
		}
	}
	bedrock_mutex_unlock(&item_mutex);

	block = block_find(id);

	if (block != NULL)
		return &block->item;

	return NULL;
}

struct item *item_find_by_name(const char *name)
{
	struct block *block;
	bedrock_node *node;

	bedrock_mutex_lock(&item_mutex);
	LIST_FOREACH(&items, node)
	{
		struct item *item = node->data;

		if (!strcmp(name, item->name))
		{
			bedrock_mutex_unlock(&item_mutex);
			return item;
		}
	}
	bedrock_mutex_unlock(&item_mutex);

	block = block_find_by_name(name);
	if (block != NULL)
		return item_find_or_create(block->item.id);

	return NULL;
}

struct item *item_find_or_create(enum item_type id)
{
	struct item *item = item_find(id);
	
	if (id > INT16_MAX)
		return NULL;

	if (item == NULL)
	{
		bedrock_log(LEVEL_DEBUG, "item: Unrecognized item %d", id);

		item = bedrock_malloc(sizeof(struct item));
		item->id = id;
		item->name = bedrock_strdup("Unknown");
		item->flags = 0;

		bedrock_mutex_lock(&item_mutex);
		bedrock_list_add(&items, item);
		bedrock_mutex_unlock(&item_mutex);
	}

	return item;
}

