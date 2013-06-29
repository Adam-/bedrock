#include "server/bedrock.h"
#include "blocks/blocks.h"
#include "server/column.h"
#include "entities/entity.h"
#include "util/string.h"
#include "util/yml.h"

#include <sys/types.h>
#include <dirent.h>

bedrock_list blocks = LIST_INIT;

void simple_drop(struct client *client, struct chunk *chunk, int32_t x, uint8_t y, int32_t z, struct block *block, bool harvest)
{
	if (!harvest || client->gamemode == GAMEMODE_CREATIVE)
		return;

	struct dropped_item *di = bedrock_malloc(sizeof(struct dropped_item));
	di->item = item_find_or_create(block->id);
	di->count = 1;
	di->data = 0;
	di->x = x;
	di->y = y;
	di->z = z;

	column_add_item(chunk->column, di);
}

static void block_free(struct block *b)
{
	bedrock_free(b->name);
	bedrock_free(b);
}

int block_init()
{
	DIR *dir = opendir("data/blocks");
	struct dirent *ent;

	blocks.free = (bedrock_free_func) block_free;

	if (!dir)
		return -1;
	
	while ((ent = readdir(dir)))
	{
		char filename[PATH_MAX];
		struct yaml_object *yaml;
		struct block *b;
		bedrock_node *node;

		if (!strstr(ent->d_name, ".yml"))
			continue;

		snprintf(filename, sizeof(filename), "data/blocks/%s", ent->d_name);
		yaml = yaml_parse(filename);
		if (yaml == NULL)
			continue;

		b = bedrock_malloc(sizeof(struct block));

		LIST_FOREACH(&yaml->objects, node)
		{
			struct yaml_object *attr = node->data;

			if (!strcmp(attr->name, "id"))
				b->id = atoi(attr->value);
			else if (!strcmp(attr->name, "name"))
				b->name = bedrock_strdup(attr->value);
			else if (!strcmp(attr->name, "hardness"))
				b->hardness = atof(attr->value);
			else if (!strcmp(attr->name, "no-harvest-time"))
				b->no_harvest_time = atof(attr->value);
			else if (!strcmp(attr->name, "weakness"))
			{
				if (!strcmp(attr->value, "pickaxe"))
					b->weakness |= ITEM_FLAG_PICKAXE;
				else if (!strcmp(attr->value, "shovel"))
					b->weakness |= ITEM_FLAG_SHOVEL;
				else if (!strcmp(attr->value, "axe"))
					b->weakness |= ITEM_FLAG_AXE;
			}
			else if (!strcmp(attr->name, "weakness-metal"))
			{
				if (!strcmp(attr->value, "gold"))
					b->weakness |= TOOL_TYPE_MASK_GOLD;
				else if (!strcmp(attr->value, "diamond"))
					b->weakness |= TOOL_TYPE_MASK_DIAMOND;
				else if (!strcmp(attr->value, "iron"))
					b->weakness |= TOOL_TYPE_MASK_IRON;
				else if (!strcmp(attr->value, "stone"))
					b->weakness |= TOOL_TYPE_MASK_STONE;
				else if (!strcmp(attr->value, "wood"))
					b->weakness |= TOOL_TYPE_MASK_WOOD;
			}
			else if (!strcmp(attr->name, "on_mine"))
				;
		}

		yaml_object_free(yaml);

		bedrock_list_add(&blocks, b);
	}
	
	closedir(dir);

	return 0;
}

void block_shutdown()
{
	bedrock_list_clear(&blocks);
}

struct block *block_find(enum block_type id)
{
	bedrock_node *node;

	LIST_FOREACH(&blocks, node)
	{
		struct block *b = node->data;

		if (b->id == id)
			return b;
	}

	return NULL;
}

struct block *block_find_by_name(const char *name)
{
	bedrock_node *node;

	LIST_FOREACH(&blocks, node)
	{
		struct block *b = node->data;

		if (!strcmp(name, b->name))
			return b;
	}

	return NULL;
}

struct block *block_find_or_create(enum block_type id)
{
	static struct block b;
	struct block *block = block_find(id);

	if (block == NULL)
	{
		bedrock_log(LEVEL_DEBUG, "block: Unrecognized block %d", id);

		b.id = id;
		b.name = "Unknown";
		b.hardness = 0;
		b.no_harvest_time = 0;
		b.weakness = ITEM_FLAG_NONE;
		b.harvest = ITEM_FLAG_NONE;
		b.on_mine = NULL;
		b.on_place = NULL;

		block = &b;
	}

	return block;
}

