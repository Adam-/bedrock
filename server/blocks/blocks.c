#include "server/bedrock.h"
#include "blocks/blocks.h"
#include "server/column.h"
#include "entities/entity.h"
#include "util/string.h"

#include <yaml.h>

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
		FILE *f;
		struct block *b;

		yaml_parser_t parser;
		yaml_token_t token;
		yaml_token_type_t type = YAML_NO_TOKEN;
		char *key = NULL;
		int done;

		if (!strstr(ent->d_name, ".yml"))
			continue;

		snprintf(filename, sizeof(filename), "data/blocks/%s", ent->d_name);
		f = fopen(filename, "rb");
		if (!f)
			continue;

		if (!yaml_parser_initialize(&parser))
			continue;

		b = bedrock_malloc(sizeof(struct block));

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
						b->id = atoi(value);
					else if (!strcmp(key, "name"))
						b->name = bedrock_strdup(value);
					else if (!strcmp(key, "hardness"))
						b->hardness = atof(value);
					else if (!strcmp(key, "no-harvest-time"))
						b->no_harvest_time = atof(value);
					else if (!strcmp(key, "weakness"))
					{
						if (!strcmp(value, "pickaxe"))
							b->weakness |= ITEM_FLAG_PICKAXE;
						else if (!strcmp(value, "shovel"))
							b->weakness |= ITEM_FLAG_SHOVEL;
						else if (!strcmp(value, "axe"))
							b->weakness |= ITEM_FLAG_AXE;
					}
					else if (!strcmp(key, "weakness-metal"))
					{
						if (!strcmp(value, "gold"))
							b->weakness |= TOOL_TYPE_MASK_GOLD;
						else if (!strcmp(value, "diamond"))
							b->weakness |= TOOL_TYPE_MASK_DIAMOND;
						else if (!strcmp(value, "iron"))
							b->weakness |= TOOL_TYPE_MASK_IRON;
						else if (!strcmp(value, "stone"))
							b->weakness |= TOOL_TYPE_MASK_STONE;
						else if (!strcmp(value, "wood"))
							b->weakness |= TOOL_TYPE_MASK_WOOD;
					}
					else if (!strcmp(key, "on_mine"))
						;
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

