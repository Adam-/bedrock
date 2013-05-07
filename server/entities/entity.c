#include "entities/entity.h"
#include "blocks/blocks.h"
#include "windows/window.h"
#include "packet/packet_open_window.h"
#include "packet/packet_set_slot.h"

/* load entities from data and put them into column */
void entity_load(struct column *column, nbt_tag *data)
{
	nbt_tag *tag = nbt_get(data, TAG_LIST, 2, "Level", "TileEntities");
	bedrock_node *node;

	LIST_FOREACH(&tag->payload.tag_list.list, node)
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
				bedrock_node *node2;

				entity = &chest->entity;
				items = nbt_get(tile_tag, TAG_LIST, 1, "Items");

				LIST_FOREACH(&items->payload.tag_list.list, node2)
				{
					nbt_tag *chest_item = node2->data;
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
		entity->data = tag;

		bedrock_list_add(&column->tile_entities, entity);

		bedrock_log(LEVEL_DEBUG, "entity: Loaded entity %s at %d, %d, %d", item->name, entity->x, entity->y, entity->z);
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

