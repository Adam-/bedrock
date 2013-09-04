#include "entities/entity.h"
#include "packet/packet_open_window.h"
#include "packet/packet_set_slot.h"
#include "packet/packet_block_change.h"
#include "packet/packet_update_window_property.h"

static void furnace_on_free(struct furnace *furnace);

/* furnaces which are burning */
static bedrock_list burning_furnaces = LIST_INIT;
static bedrock_mutex furnace_mutex;

void furnace_init()
{
	bedrock_mutex_init(&furnace_mutex, "furnace mutex");
}

struct tile_entity *furnace_load(struct column *column, nbt_tag *tag)
{
	struct furnace *furnace = bedrock_malloc(sizeof(struct furnace));
	bedrock_node *node;
	nbt_tag *items;
	uint8_t *block;
	int32_t x, y, z;

	nbt_copy(tag, TAG_SHORT, &furnace->fuel_indicator, sizeof(furnace->fuel_indicator), 1, "BurnTime");
	nbt_copy(tag, TAG_SHORT, &furnace->progress_arrow, sizeof(furnace->progress_arrow), 1, "CookTime");

	items = nbt_get(tag, TAG_LIST, 1, "Items");

	LIST_FOREACH(&items->payload.tag_list.list, node)
	{
		nbt_tag *furnace_item = node->data;
		struct item_stack *item;

		int16_t id, damage;
		int8_t count, slot;
		
		nbt_copy(furnace_item, TAG_SHORT, &id, sizeof(id), 1, "id");
		nbt_copy(furnace_item, TAG_SHORT, &damage, sizeof(damage), 1, "Damage");
		nbt_copy(furnace_item, TAG_BYTE, &count, sizeof(count), 1, "Count");
		nbt_copy(furnace_item, TAG_BYTE, &slot, sizeof(slot), 1, "Slot");

		bedrock_assert(slot >= 0 && slot <= 2, continue);

		switch (slot)
		{
			case FURNACE_INPUT:
				item = &furnace->in;
				break;
			case FURNACE_FUEL:
				item = &furnace->fuel;
				break;
			case FURNACE_OUTPUT:
				item = &furnace->out;
				break;
			default:
				continue;
		}

		item->id = id;
		item->count = count;
		item->metadata = damage;
	}

	furnace->entity.on_free = (void (*)(struct tile_entity *)) furnace_on_free;

	nbt_copy(tag, TAG_INT, &x, sizeof(x), 1, "x");
	nbt_copy(tag, TAG_INT, &y, sizeof(y), 1, "y");
	nbt_copy(tag, TAG_INT, &z, sizeof(z), 1, "z");
	block = column_get_block(column, x, y, z);
	if (block && *block == BLOCK_BURNING_FURNACE)
	{
		furnace->burning = true;
		bedrock_mutex_lock(&furnace_mutex);
		bedrock_list_add(&burning_furnaces, furnace);
		bedrock_mutex_unlock(&furnace_mutex);
	}

	bedrock_assert(!offsetof(struct furnace, entity), ;);
	return &furnace->entity;
}

void furnace_save(nbt_tag *entity_tag, struct tile_entity *entity)
{
	struct furnace *furnace = (struct furnace *) entity;
	struct item_stack *stacks[] = { &furnace->in, &furnace->fuel, &furnace->out };
	nbt_tag *items;
	int i;

	nbt_add(entity_tag, TAG_SHORT, "BurnTime", &furnace->fuel_indicator, sizeof(furnace->fuel_indicator));
	nbt_add(entity_tag, TAG_SHORT, "CookTime", &furnace->progress_arrow, sizeof(furnace->progress_arrow));

	items = nbt_add(entity_tag, TAG_LIST, "Items", NULL, 0);

	for (i = 0; i < ENTITY_FURNACE_SLOTS; ++i)
	{
		struct item_stack *stack = stacks[i];
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

static void furnace_on_free(struct furnace *furnace)
{
	bedrock_mutex_lock(&furnace_mutex);
	bedrock_list_del(&burning_furnaces, furnace);
	bedrock_mutex_unlock(&furnace_mutex);
}

void furnace_operate(struct client *client, struct tile_entity *entity)
{
	struct furnace *furnace = (struct furnace *) entity;
	struct item_stack *stacks[] = { &furnace->in, &furnace->fuel, &furnace->out };
	int i;

	packet_send_open_window(client, WINDOW_FURNACE, NULL, ENTITY_FURNACE_SLOTS);

	for (i = 0; i < 3; ++i)
	{
		struct item_stack *stack = stacks[i];

		if (stack->id == 0)
			continue;

		packet_send_set_slot(client, client->window_data.id, i, item_find_or_create(stack->id), stack->count, stack->metadata);
	}
}

void furnace_propagate(struct furnace *furnace)
{
	bedrock_node *node;
	struct item_stack *stacks[3];

	bedrock_assert(furnace != NULL, return);

	if (!furnace->in.count)
		furnace->progress_arrow = 0;
	if (!furnace->fuel.count)
		furnace->fuel_indicator = 0;

	stacks[0] = &furnace->in, stacks[1] = &furnace->fuel, stacks[2] = &furnace->out;

	LIST_FOREACH(&furnace->entity.clients, node)
	{
		struct client *c = node->data;
		int i;
		struct item *item;

		for (i = 0; i < 3; ++i)
		{
			struct item_stack *stack = stacks[i];

			if (stack->id)
				packet_send_set_slot(c, c->window_data.id, i, item_find_or_create(stack->id), stack->count, stack->metadata);
			else
				packet_send_set_slot(c, c->window_data.id, i, NULL, 0, 0);
		}

		packet_send_update_window_property(c, c->window_data.id, WINDOW_PROPERTY_FURNACE_PROGRESS_ARROW, furnace->progress_arrow);

		item = item_find(furnace->fuel.id);
		if (item && item->furnace_burn_time)
			packet_send_update_window_property(c, c->window_data.id, WINDOW_PROPERTY_FURNACE_FIRE_ICON, (float) furnace->fuel_indicator / (float) item->furnace_burn_time * 200.0);
		else
			packet_send_update_window_property(c, c->window_data.id, WINDOW_PROPERTY_FURNACE_FIRE_ICON, 0);
	}

	furnace_burn(furnace);
}

void furnace_burn(struct furnace *furnace)
{
	struct item *item = item_find(furnace->fuel.id);
	bedrock_node *node;

	if (item == NULL || !item->furnace_burn_time)
	{
		if (furnace->burning)
		{
			/* 'block change' the furnace to the not lit version */
			uint8_t *block = column_get_block(furnace->entity.column, furnace->entity.x, furnace->entity.y, furnace->entity.z);
			if (block)
			{
				*block = BLOCK_FURNACE;
				column_set_pending(furnace->entity.column, COLUMN_FLAG_DIRTY);
			}

			LIST_FOREACH(&furnace->entity.column->players, node)
			{
				struct client *c = node->data;

				packet_send_block_change(c, furnace->entity.x, furnace->entity.y, furnace->entity.z, BLOCK_FURNACE, 0); /// XXX direction
			}

			bedrock_list_del(&burning_furnaces, furnace);
			furnace->burning = false;
		}
	}
	else
	{
		if (furnace->burning)
			return;

		/* 'block change' the furnace to the lit version */
		uint8_t *block = column_get_block(furnace->entity.column, furnace->entity.x, furnace->entity.y, furnace->entity.z);
		if (block)
		{
			*block = BLOCK_BURNING_FURNACE;
			column_set_pending(furnace->entity.column, COLUMN_FLAG_DIRTY);
		}

		LIST_FOREACH(&furnace->entity.column->players, node)
		{
			struct client *c = node->data;

			packet_send_block_change(c, furnace->entity.x, furnace->entity.y, furnace->entity.z, BLOCK_BURNING_FURNACE, 0); // XXX direction
		}

		bedrock_list_add(&burning_furnaces, furnace);
		furnace->burning = true;
	}
}

void furnace_tick(uint64_t diff)
{
	bedrock_node *node, *node2;

	bedrock_mutex_lock(&furnace_mutex);
	LIST_FOREACH_SAFE(&burning_furnaces, node, node2)
	{
		struct furnace *furnace = node->data;
		struct item *item = item_find(furnace->fuel.id);
		int used_fuel;

		if (item == NULL || !item->furnace_burn_time)
		{
			furnace_burn(furnace);
			continue;
		}

		furnace->fuel_indicator += diff;
		used_fuel = furnace->fuel_indicator / item->furnace_burn_time;
		furnace->fuel_indicator %= item->furnace_burn_time;

		if (used_fuel >= furnace->fuel.count)
			used_fuel = furnace->fuel.count;

		furnace->fuel.count -= used_fuel;
		if (!furnace->fuel.count)
		{
			/* fuel goes away */
			furnace->fuel.id = 0;
			furnace->fuel.metadata = 0;
			furnace->fuel_indicator = 0;
		}

		item = item_find(furnace->in.id);
		if (item != NULL && item->furnace_output && furnace->fuel.count)
		{
			int new_items, can_hold;

			furnace->progress_arrow += diff;
			new_items = furnace->progress_arrow / 200;
			furnace->progress_arrow %= 200;

			if (furnace->out.id && item->furnace_output != furnace->out.id)
			{
				/* Output stack does not match what this is creating, don't create anything */
			}
			else
			{
				can_hold = BEDROCK_MAX_ITEMS_PER_STACK - furnace->out.count; // Output can hold this many more items
				if (new_items > can_hold)
					new_items = can_hold;

				if (new_items >= furnace->in.count)
				{
					/* All input goes away */
					furnace->in.id = 0;
					furnace->in.count = 0;
					furnace->in.metadata = 0;
				}
				else
					/* Remove items from input */
					furnace->in.count -= new_items;

				/* Add new items to output */
				if (new_items)
				{
					furnace->out.id = item->furnace_output;
					furnace->out.count += new_items;
				}
			}
		}

		/* Propagate changes to the various slots/indicators */
		furnace_propagate(furnace);
	}
	bedrock_mutex_unlock(&furnace_mutex);
}

