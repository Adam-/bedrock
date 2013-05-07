#include "server/bedrock.h"
#include "server/client.h"
#include "server/column.h"
#include "server/packet.h"
#include "blocks/blocks.h"
#include "entities/entity.h"
#include "nbt/nbt.h"
#include "windows/window.h"
#include "packet/packet_confirm_transaction.h"

#include <math.h>

enum
{
	BUTTON_LEFT_START = 0,
	BUTTON_LEFT = 0,
	BUTTON_LEFT_PAINT = 1,
	BUTTON_RIGHT = 1,
	BUTTON_LEFT_END = 2,
	BUTTON_MIDDLE = 3,
	BUTTON_RIGHT_START = 4,
	BUTTON_RIGHT_PAINT = 5,
	BUTTON_RIGHT_END = 6
};

enum
{
	OP_REGULAR,
	OP_SHIFT,
	OP_PAINT = 5,
	OP_DOUBLE_CLICK
};

#define MAX_SLOTS 89 // One window can't contain more than this many slots.

int packet_click_window(struct client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint8_t window;
	int16_t slot;
	uint8_t button;
	uint16_t action;
	uint8_t mode;
	struct item_stack slot_data;
	bool ok = true;

	struct item_stack *stack = NULL;
	struct item_stack *slots[MAX_SLOTS];
	int i;

	packet_read_int(p, &offset, &window, sizeof(window));
	packet_read_int(p, &offset, &slot, sizeof(slot));
	packet_read_int(p, &offset, &button, sizeof(button));
	packet_read_int(p, &offset, &action, sizeof(action));
	packet_read_int(p, &offset, &mode, sizeof(mode));
	packet_read_slot(p, &offset, &slot_data);

	for (i = 0; i < MAX_SLOTS; ++i)
		slots[i] = NULL;

	if (slot == -999)
		; // Outside of the window
	else
	{
		if (window == WINDOW_INVENTORY)
		{
			if (slot < INVENTORY_CRAFT_OUTPUT || slot > INVENTORY_SIZE)
				return ERROR_NOT_ALLOWED;
	
			for (i = INVENTORY_CRAFT_OUTPUT; i < INVENTORY_SIZE; ++i)
				slots[i] = &client->inventory[i];
		}
		else if (window != client->window_data.id)
			return ERROR_NOT_ALLOWED;
		else if (client->window_data.type == WINDOW_CHEST)
		{
			struct column *column = find_column_from_world_which_contains(client->world, client->window_data.x, client->window_data.z);
			struct chest *chest;

			if (column == NULL)
				return ERROR_UNEXPECTED;

			chest = (struct chest *) column_find_tile_entity(column, BLOCK_CHEST, client->window_data.x, client->window_data.y, client->window_data.z);
			if (chest == NULL)
				return ERROR_UNEXPECTED;

			if (slot < 0 || slot >= CHEST_SIZE)
				return ERROR_NOT_ALLOWED;

			for (i = 0; i < CHEST_INVENTORY_START; ++i)
				slots[i] = &chest->items[i];
			for (i = 0; i < INVENTORY_SIZE - INVENTORY_START; ++i)
				slots[i + CHEST_INVENTORY_START] = &client->inventory[INVENTORY_START + i];
		}
		else
			return ERROR_UNEXPECTED;

		bedrock_assert(slot >= 0 && slot < MAX_SLOTS, return ERROR_NOT_ALLOWED);
		stack = slots[slot];
		bedrock_assert(stack != NULL, return ERROR_NOT_ALLOWED);

		bedrock_log(LEVEL_DEBUG, "click window: %s clicked slot %d clicked which contains %d %s", client->name, slot, stack->count, item_find_or_create(stack->id)->name);

		for (i = 0; i < MAX_SLOTS; ++i)
			if (slots[i] != NULL && slots[i]->id)
				 bedrock_log(LEVEL_DEBUG, "click window: Slot %d contains %d %s", i, slots[i]->count, item_find_or_create(slots[i]->id)->name);
	}

	// I clicked a valid slot with items in it
	if (stack != NULL && stack->count)
	{
		if (slot_data.id == -1 && slot_data.count == 0)
			/* Painting sends this in slot data */;
		else if (stack->id != slot_data.id || stack->count != slot_data.count) // XXX I have no metadata tracking?
			return ERROR_UNEXPECTED; // This is a client/server desync

		// If I am already dragging an item replace it with this slot completely, even if I right clicked this slot.
		if (client->drag_data.stack.id)
		{
			/* Except if the IDs match */
			if (stack->id == client->drag_data.stack.id)
			{
				if (mode == OP_PAINT)
				{
					/* Painting on to a stack of existing matching items */
					if (button == BUTTON_RIGHT_PAINT)
					{
						if (client->drag_data.stack.count && stack->count < BEDROCK_MAX_ITEMS_PER_STACK)
						{
							bedrock_log(LEVEL_DEBUG, "click window: %s paints 1 block of %s in to slot %d", client->name, item_find_or_create(client->drag_data.stack.id)->name, slot);
								--client->drag_data.stack.count;
							++stack->count;
						}
					}
					else if (button == BUTTON_LEFT_PAINT)
					{
						if (client->drag_data.slots.count < BEDROCK_MAX_ITEMS_PER_STACK)
						{
							int *i = bedrock_malloc(sizeof(int));
							*i = slot;

							bedrock_list_add(&client->drag_data.slots, i);
							client->drag_data.slots.free = bedrock_free;

							bedrock_log(LEVEL_DEBUG, "click window: %s paints across slot %d", client->name, slot);
						}
						else
							/* Painting to too many slots */
							ok = false;
					}
				}
				/* On right click take one item, else combine them as much as possible */
				else if (button == BUTTON_RIGHT)
				{
					if (stack->count < BEDROCK_MAX_ITEMS_PER_STACK && client->drag_data.stack.count)
					{
						++stack->count;
						--client->drag_data.stack.count;

						bedrock_log(LEVEL_DEBUG, "click window: %s moves one item from stack of %s to %d, has %d left", client->name, item_find_or_create(client->drag_data.stack.id)->name, slot, client->drag_data.stack.count);

						// Might have been the last item, zero out drag state
						if (client->drag_data.stack.count == 0)
						{
							client->drag_data.stack.id = 0;
							client->drag_data.stack.metadata = 0;
						}
					}
				}
				/* On left click combine the items as much as possible */
				else if (button == BUTTON_LEFT)
				{
					int can_hold = BEDROCK_MAX_ITEMS_PER_STACK - stack->count;

					/* First check that we can hold items and that we have items */
					if (can_hold && client->drag_data.stack.count)
					{
						/* If we can hold more than is in this stack, take the whole stack */
						if (can_hold >= client->drag_data.stack.count)
						{
							bedrock_log(LEVEL_DEBUG, "click window: %s adds %d items to slot %d", client->name, client->drag_data.stack.count, slot);

							stack->count += client->drag_data.stack.count;

							/* Zero out drag state */
							client->drag_data.stack.id = 0;
							client->drag_data.stack.count = 0;
							client->drag_data.stack.metadata = 0;
						}
						else
						{
							/* Only take what we can hold */
							stack->count += can_hold;
							client->drag_data.stack.count -= can_hold;

							bedrock_log(LEVEL_DEBUG, "click window: %s adds %d items to slot %d, and keeps %d", client->name, can_hold, slot, client->drag_data.stack.count);
						}
					}
				}
				/* unknown button/mode? */
				else
					ok = false;
			}
			/* Trying to paint to an existing stack that doesn't match? */
			else if (mode == OP_PAINT)
				return ERROR_UNEXPECTED;
			// Replacing a slot
			else
			{
				bedrock_log(LEVEL_DEBUG, "click window: %s replaces slot %d with %d blocks of %s", client->name, slot, client->drag_data.stack.count, item_find_or_create(client->drag_data.stack.id)->name);

				/* Swap the drag data with this slot */
				*stack = client->drag_data.stack;

				/* We are now dragging what was in this slot */
				client->drag_data.stack = slot_data;
			}
		}
		else
		{
			/* This item is now being dragged and is no longer in its slot */
			client->drag_data.stack.id = stack->id;
			client->drag_data.stack.metadata = stack->metadata;

			if (button == BUTTON_RIGHT)
			{
				/* We only want half of the blocks here. If there is an odd count then they are holding the larger. */
				double dcount = (double) stack->count / 2;
				bool even = floor(dcount) == dcount;

				// If this isn't an even outcome give the client one more
				if (even == false)
					++dcount;

				client->drag_data.stack.count = dcount;
				bedrock_log(LEVEL_DEBUG, "click window: %s right clicks on slot %d which contains %s and takes %d", client->name, slot, item_find_or_create(stack->id)->name, client->drag_data.stack.count);

				stack->count /= 2;

				// Slot can be empty (right clicking a 1 item slot)
				if (stack->count == 0)
				{
					stack->id = 0;
					stack->metadata = 0;
				}
			}
			else if (button == BUTTON_LEFT)
			{
				client->drag_data.stack.count = stack->count;
				bedrock_log(LEVEL_DEBUG, "click window: %s clicks on slot %d which contains %s and takes all %d blocks", client->name, slot, item_find_or_create(stack->id)->name, stack->count);

				/* Slot is now empty */
				stack->id = 0;
				stack->count = 0;
				stack->metadata = 0;
			}
			/* unknown button? */
			else
				ok = false;
		}
	}
	else
	{
		if (stack == NULL)
		{
			if (mode == OP_PAINT)
			{
				/* Trying to (stop) paint with nothing held? */
				if (!client->drag_data.stack.id)
					return ERROR_UNEXPECTED;

				if (button == BUTTON_LEFT_START || button == BUTTON_RIGHT_START)
				{
					bedrock_log(LEVEL_DEBUG, "click window: %s starts painting with %s using %s click", client->name, item_find_or_create(client->drag_data.stack.id)->name, button == BUTTON_LEFT_START ? "left" : "right");
				}
				else if (button == BUTTON_LEFT_END || button == BUTTON_RIGHT_END)
				{
					if (button == BUTTON_LEFT_END)
					{
						/* Not we apply the move */
							
						if (client->drag_data.slots.count)
						{
							/* Each of the slots wants this amount */
							int each = client->drag_data.stack.count / client->drag_data.slots.count;
							bedrock_node *node;

							LIST_FOREACH(&client->drag_data.slots, node)
							{
								int *i = node->data;
								int can_hold, can_give, will_give;

								bedrock_assert(*i > 0 && *i < MAX_SLOTS, return ERROR_UNEXPECTED);
								stack = slots[*i];

								/* Client trying to paint an item over a slot of a different type */
								if (stack == NULL || (stack->id && stack->id != client->drag_data.stack.id))
									return ERROR_NOT_ALLOWED;

								/* Slot can hold this many more items */
								can_hold = BEDROCK_MAX_ITEMS_PER_STACK - stack->count;
								/* I should give at most this much from what I'm holding */
								can_give = client->drag_data.stack.count < each ? client->drag_data.stack.count : each;
								/* I will give this much */
								will_give = can_hold < can_give ? can_hold : can_give;

								bedrock_log(LEVEL_DEBUG, "click window: %s paints %d items to slot %d", client->name, will_give, *i);

								stack->id = client->drag_data.stack.id;
								stack->count += will_give;

								client->drag_data.stack.count -= will_give;
							}

							/* Clear pending slot data */
							bedrock_list_clear(&client->drag_data.slots);
						}
					}

					bedrock_log(LEVEL_DEBUG, "click window: %s stops painting with %s using %s click", client->name, item_find_or_create(client->drag_data.stack.id)->name, button == BUTTON_LEFT_END ? "left" : "right");

					/* Zero drag data if we don't have any items left, otherwise keep holding the items */
					if (!client->drag_data.stack.count)
					{
						client->drag_data.stack.id = 0;
						client->drag_data.stack.metadata = 0;
					}
				}
				/* unknown button */
				else
					ok = false;
			}
			// Dropping items on the ground
			else if (client->drag_data.stack.id)
			{
				/* Spawn dropped item */
				struct dropped_item *di = bedrock_malloc(sizeof(struct dropped_item));
				struct column *col;

				di->item = item_find_or_create(client->drag_data.stack.id);
				di->count = client->drag_data.stack.count;
				di->data = client->drag_data.stack.metadata;
				di->x = *client_get_pos_x(client);
				di->y = *client_get_pos_y(client);
				di->z = *client_get_pos_z(client);

				// XXX put in the direction the user is facing
				di->x += rand() % 4;
				di->z += rand() % 4;

				col = find_column_from_world_which_contains(client->world, di->x, di->z);
				if (col != NULL)
					column_add_item(client->column, di);
				else
					bedrock_free(di);

				bedrock_log(LEVEL_DEBUG, "click window: %s drops %d blocks of %s", client->name, client->drag_data.stack.count, item_find_or_create(client->drag_data.stack.id)->name);

				/* Zero drag state */
				client->drag_data.stack.id = 0;
				client->drag_data.stack.count = 0;
				client->drag_data.stack.metadata = 0;
			}
			/* clicking outside of inventory while not holding anything */
			else
				ok = false;
		}
		// Clicked an empty slot, might be placing blocks there, if we are currently dragging something
		else if (client->drag_data.stack.id)
		{
			if (mode == OP_DOUBLE_CLICK)
			{
				/* On double click take items from inventory to fill up what I'm holding until it's full */
				for (i = 0; slots[i] != NULL && i < MAX_SLOTS; ++i)
				{
					stack = slots[i];

					if (stack && stack->id == client->drag_data.stack.id)
					{
						/* I can hold this many more items */
						int can_hold = BEDROCK_MAX_ITEMS_PER_STACK - client->drag_data.stack.count;
						int will_take = can_hold < stack->count ? can_hold : stack->count;

						client->drag_data.stack.count += will_take;
						stack->count -= will_take;

						/* Slot might be empty now */
						if (!stack->count)
						{
							stack->id = 0;
							stack->metadata = 0;
						}
					}
				}

				bedrock_log(LEVEL_DEBUG, "click window: %s double clicks and now holds %d items", client->name, client->drag_data.stack.count);
			}
			/* Client is painting */
			else if (mode == OP_PAINT)
			{
				if (button == BUTTON_RIGHT_PAINT)
				{
					if (client->drag_data.stack.count)
					{
						bedrock_log(LEVEL_DEBUG, "click window: %s paints 1 block of %s in to empty slot %d", client->name, item_find_or_create(client->drag_data.stack.id)->name, slot);

						--client->drag_data.stack.count;
						stack->count = 1;
						stack->id = client->drag_data.stack.id;
					}
				}
				else if (button == BUTTON_LEFT_PAINT)
				{
					if (client->drag_data.slots.count < BEDROCK_MAX_ITEMS_PER_STACK)
					{
						int *i = bedrock_malloc(sizeof(int));
						*i = slot;

						bedrock_list_add(&client->drag_data.slots, i);
						client->drag_data.slots.free = bedrock_free;

						bedrock_log(LEVEL_DEBUG, "click window: %s paints across empty slot %d", client->name, slot);
					}
					else
						/* Painting to too many slots */
						ok = false;
				}
				else
					ok = false;
			}
			else
			{
				/* Replace slot with our drag data */
				stack->id = client->drag_data.stack.id;
				stack->metadata = client->drag_data.stack.metadata;

				// On a right click we only transfer one item, otherwise take them all
				if (button == BUTTON_RIGHT && client->drag_data.stack.count)
				{
					stack->count = 1;
					--client->drag_data.stack.count;

					bedrock_log(LEVEL_DEBUG, "click window: %s right click places 1 block of %s in empty slot %d", client->name, item_find_or_create(client->drag_data.stack.id)->name, slot);
				}
				else
				{
					stack->count = client->drag_data.stack.count;
					bedrock_log(LEVEL_DEBUG, "click window: %s places %d blocks of %s in empty slot %d", client->name, client->drag_data.stack.count, item_find_or_create(client->drag_data.stack.id)->name, slot);
				}

				// Zero out drag state if this wasn't a right click replace or if there are no items left
				if (button != BUTTON_RIGHT || client->drag_data.stack.count == 0)
				{
					client->drag_data.stack.id = 0;
					client->drag_data.stack.count = 0;
					client->drag_data.stack.metadata = 0;
				}
			}
		}
		/* clicked an empty slot but not dragging anything */
		else
			ok = false;
	}

	packet_send_confirm_transaction(client, window, action, ok);

	return offset;
}
