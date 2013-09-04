#include "entities/entity.h"

void entity_init()
{
	furnace_init();
}

/* load entities from data and put them into column */
void entity_load(struct column *column, nbt_tag *data)
{
	nbt_tag *tag = nbt_get(data, TAG_LIST, 2, "Level", "TileEntities");
	bedrock_node *node, *node2;

	LIST_FOREACH_SAFE(&tag->payload.tag_list.list, node, node2)
	{
		nbt_tag *tile_tag = node->data;
		nbt_tag *tile_id_tag = nbt_get(tile_tag, TAG_STRING, 1, "id");
		struct tile_entity *entity;

		struct item *item = item_find_by_name(tile_id_tag->payload.tag_string);
		if (item == NULL)
			continue;

		switch (item->id)
		{
			case BLOCK_CHEST:
				entity = chest_load(tile_tag);
				break;
			case BLOCK_FURNACE:
				entity = furnace_load(column, tile_tag);
				break;
			default:
				continue;
		}

		entity->blockid = item->id;
		nbt_copy(tile_tag, TAG_INT, &entity->x, sizeof(entity->x), 1, "x");
		nbt_copy(tile_tag, TAG_INT, &entity->y, sizeof(entity->y), 1, "y");
		nbt_copy(tile_tag, TAG_INT, &entity->z, sizeof(entity->z), 1, "z");

		nbt_free(tile_tag);

		entity->column = column;
		bedrock_list_add(&column->tile_entities, entity);

		bedrock_log(LEVEL_DEBUG, "entity: Loaded entity %s at %d, %d, %d", item->name, entity->x, entity->y, entity->z);
	}
}

/* save tile entities back to the column */
void entity_save(struct column *column)
{
	bedrock_node *node;
	nbt_tag *tile_entities = nbt_get(column->data, TAG_LIST, 2, "Level", "TileEntities");

	LIST_FOREACH(&column->tile_entities, node)
	{
		struct tile_entity *entity = node->data;
		struct item *item = item_find(entity->blockid);
		nbt_tag *entity_tag;

		if (item == NULL)
			continue;

		entity_tag = nbt_add(tile_entities, TAG_COMPOUND, NULL, NULL, 0);

		nbt_add(entity_tag, TAG_STRING, "id", item->name, strlen(item->name));
		nbt_add(entity_tag, TAG_INT, "x", &entity->x, sizeof(entity->x));
		nbt_add(entity_tag, TAG_INT, "y", &entity->y, sizeof(entity->y));
		nbt_add(entity_tag, TAG_INT, "z", &entity->z, sizeof(entity->z));

		switch (entity->blockid)
		{
			case BLOCK_CHEST:
				chest_save(entity_tag, entity);
				break;
			case BLOCK_FURNACE:
				furnace_save(entity_tag, entity);
				break;
		}
	}
}

void entity_cleanup(struct column *column)
{
	nbt_tag *tile_entities = nbt_get(column->data, TAG_LIST, 2, "Level", "TileEntities");
	bedrock_node *node, *node2;

	LIST_FOREACH_SAFE(&tile_entities->payload.tag_list.list, node, node2)
	{
		nbt_tag *entity = node->data;

		const char *id = nbt_read_string(entity, 1, "id");

		if (id == NULL)
			continue;

		if (!strcmp(id, "Chest") || !strcmp(id, "Furnace"))
			nbt_free(entity);
	}
}

void entity_free(struct tile_entity *entity)
{
	bedrock_node *node;

	if (entity->on_free != NULL)
		entity->on_free(entity);

	bedrock_list_del(&entity->column->tile_entities, entity);

	LIST_FOREACH(&entity->clients, node)
	{
		struct client *c = node->data;
		c->window_data.entity = NULL;
	}

	bedrock_free(entity);
}

void entity_operate(struct client *client, struct tile_entity *entity)
{
	switch (entity->blockid)
	{
		case BLOCK_CHEST:
			chest_operate(client, entity);
			break;
		case BLOCK_FURNACE:
			furnace_operate(client, entity);
			break;
	}
	
	if (client->window_data.entity != NULL)
		bedrock_list_del(&client->window_data.entity->clients, client);

	client->window_data.entity = entity;
	bedrock_list_add(&entity->clients, client);

	switch (entity->blockid)
	{
		case BLOCK_CHEST:
			chest_propagate((struct chest *) entity);
			break;
		case BLOCK_FURNACE:
			furnace_propagate((struct furnace *) entity);
			break;
	}
}

