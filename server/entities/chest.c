#include "entities/entity.h"
#include "packet/packet_open_window.h"
#include "packet/packet_set_slot.h"

struct tile_entity *chest_load(nbt_tag *tag)
{
	struct chest *chest = bedrock_malloc(sizeof(struct chest));
	nbt_tag *items;
	bedrock_node *node;

	items = nbt_get(tag, TAG_LIST, 1, "Items");

	LIST_FOREACH(&items->payload.tag_list.list, node)
	{
		nbt_tag *chest_item = node->data;
		uint8_t slot;
		struct item_stack *stack;

		nbt_copy(chest_item, TAG_BYTE, &slot, sizeof(slot), 1, "Slot");
		bedrock_assert(slot < ENTITY_CHEST_SLOTS, continue);
					
		stack = &chest->items[slot];
					
		nbt_copy(chest_item, TAG_SHORT, &stack->id, sizeof(stack->id), 1, "id");
		nbt_copy(chest_item, TAG_SHORT, &stack->metadata, sizeof(stack->metadata), 1, "Damage");
		nbt_copy(chest_item, TAG_BYTE, &stack->count, sizeof(stack->count), 1, "Count");
	}

	bedrock_assert(!offsetof(struct chest, entity), ;);
	return &chest->entity;
}

void chest_save(nbt_tag *entity_tag, struct tile_entity *entity)
{
	struct chest *chest = (struct chest *) entity;
	nbt_tag *items;
	int i;

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

void chest_operate(struct client *client, struct tile_entity bedrock_attribute_unused *entity)
{
	packet_send_open_window(client, WINDOW_CHEST, NULL, ENTITY_CHEST_SLOTS);
}

void chest_mine(struct client *client, struct chunk *chunk, int32_t x, uint8_t y, int32_t z, struct block *block, bool harvest)
{
	struct chest *chest;
	
	simple_drop(client, chunk, x, y, z, block, harvest);

	chest = (struct chest *) column_find_tile_entity(chunk->column, BLOCK_CHEST, x, y, z);
	if (chest != NULL)
	{
		int i;
		for (i = 0; i < ENTITY_CHEST_SLOTS; ++i)
		{
			struct item_stack *item = &chest->items[i];

			if (!item->count)
				continue;

			struct dropped_item *di = bedrock_malloc(sizeof(struct dropped_item));
			di->item = item_find_or_create(item->id);
			di->count = item->count;
			di->data = item->metadata;
			di->x = x;
			di->y = y;
			di->z = z;
			column_add_item(chunk->column, di);
		}

		entity_free(&chest->entity);
	}
}

void chest_place(struct client bedrock_attribute_unused *client, struct chunk *chunk, int32_t x, uint8_t y, int32_t z, struct block bedrock_attribute_unused *block)
{
	struct chest *chest = bedrock_malloc(sizeof(struct chest));
	chest->entity.blockid = BLOCK_CHEST;
	chest->entity.x = x;
	chest->entity.y = y;
	chest->entity.z = z;
	bedrock_list_add(&chunk->column->tile_entities, chest);
}

void chest_propagate(struct chest *chest)
{
	/* contents of chest might have changed, sync changes with clients who are viewing it */
	bedrock_node *node;

	bedrock_assert(chest != NULL, return);

	LIST_FOREACH(&chest->entity.clients, node)
	{
		struct client *c = node->data;
		int i;

		for (i = 0; i < ENTITY_CHEST_SLOTS; ++i)
		{
			struct item_stack *stack = &chest->items[i];

			if (stack->id)
				packet_send_set_slot(c, c->window_data.id, i, item_find_or_create(stack->id), stack->count, stack->metadata);
			else
				packet_send_set_slot(c, c->window_data.id, i, NULL, 0, 0);
		}
	}
}

