#include "entities/entity.h"
#include "blocks/blocks.h"
#include "windows/window.h"
#include "packet/packet_open_window.h"
#include "packet/packet_set_slot.h"

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
			{
				struct chest *chest = bedrock_malloc(sizeof(struct chest));
				nbt_tag *items;
				bedrock_node *node3;

				entity = &chest->entity;
				items = nbt_get(tile_tag, TAG_LIST, 1, "Items");

				LIST_FOREACH(&items->payload.tag_list.list, node3)
				{
					nbt_tag *chest_item = node3->data;
					uint8_t slot;
					struct item_stack *stack;

					nbt_copy(chest_item, TAG_BYTE, &slot, sizeof(slot), 1, "Slot");
					bedrock_assert(slot < ENTITY_CHEST_SLOTS, continue);
					
					stack = &chest->items[slot];
					
					nbt_copy(chest_item, TAG_SHORT, &stack->id, sizeof(stack->id), 1, "id");
					nbt_copy(chest_item, TAG_SHORT, &stack->metadata, sizeof(stack->metadata), 1, "Damage");
					nbt_copy(chest_item, TAG_BYTE, &stack->count, sizeof(stack->count), 1, "Count");
				}

				break;
			}
			default:
				continue;
		}

		entity->blockid = item->id;
		nbt_copy(tile_tag, TAG_INT, &entity->x, sizeof(entity->x), 1, "x");
		nbt_copy(tile_tag, TAG_INT, &entity->y, sizeof(entity->y), 1, "y");
		nbt_copy(tile_tag, TAG_INT, &entity->z, sizeof(entity->z), 1, "z");

		nbt_free(tile_tag);

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
		struct chest *chest;
		nbt_tag *entity_tag, *items;
		int i;

		if (entity->blockid != BLOCK_CHEST)
			continue;

		chest = (struct chest *) entity;

		entity_tag = nbt_add(tile_entities, TAG_COMPOUND, NULL, NULL, 0);

		nbt_add(entity_tag, TAG_STRING, "id", "Chest", strlen("Chest"));
		nbt_add(entity_tag, TAG_INT, "x", &entity->x, sizeof(entity->x));
		nbt_add(entity_tag, TAG_INT, "y", &entity->y, sizeof(entity->y));
		nbt_add(entity_tag, TAG_INT, "z", &entity->z, sizeof(entity->z));

		items = nbt_add(entity_tag, TAG_LIST, "Items", NULL, 0);

		for (i = 0; i < ENTITY_CHEST_SLOTS; ++i)
		{
			struct item_stack *stack = &chest->items[i];
			nbt_tag *item;

			if (stack->count == 0)
				continue;

			item = nbt_add(items, TAG_COMPOUND, NULL, NULL, 0);

			nbt_add(item, TAG_SHORT, "id", &stack->id, sizeof(stack->id));
			nbt_add(item, TAG_SHORT, "Damage", &stack->metadata, sizeof(stack->metadata));
			nbt_add(item, TAG_BYTE, "Count", &stack->count, sizeof(stack->count));
			nbt_add(item, TAG_BYTE, "Slot", &i, sizeof(i));
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
		if (id && !strcmp(id, "Chest"))
			nbt_free(entity);
	}
}

void entity_free(struct tile_entity *entity)
{
	bedrock_free(entity);
}

void entity_operate(struct client *client, struct tile_entity *entity)
{
	switch (entity->blockid)
	{
		case BLOCK_CHEST:
		{
			struct chest *chest = (struct chest *) entity;
			int i;

			packet_send_open_window(client, WINDOW_CHEST, NULL, ENTITY_CHEST_SLOTS);

			client->window_data.x = entity->x;
			client->window_data.y = entity->y;
			client->window_data.z = entity->z;

			for (i = 0; i < ENTITY_CHEST_SLOTS; ++i)
			{
				struct item_stack *stack = &chest->items[i];

				if (stack->id == 0)
					continue;

				packet_send_set_slot(client, client->window_data.id, i, item_find_or_create(stack->id), stack->count, stack->metadata);
			}
			break;
		}
	}
}

