#include "server/bedrock.h"
#include "server/client.h"
#include "server/column.h"
#include "server/packet.h"
#include "packet/packet_confirm_transaction.h"
#include "nbt/nbt.h"
#include "windows/window.h"

#include <math.h>

enum
{
	BUTTON_LEFT_START,
	BUTTON_LEFT = 0,
	BUTTON_RIGHT,
	BUTTON_LEFT_END,
	BUTTON_MIDDLE,
	BUTTON_RIGHT_START,
	BUTTON_RIGHT_END = 6
};

enum
{
	OP_REGULAR,
	OP_SHIFT,
	OP_PAINT = 5,
	OP_DOUBLE_CLICK
};

int packet_click_window(struct client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint8_t window;
	int16_t slot;
	uint8_t button;
	uint16_t action;
	uint8_t mode;
	struct item_stack slot_data;

	packet_read_int(p, &offset, &window, sizeof(window));
	packet_read_int(p, &offset, &slot, sizeof(slot));
	packet_read_int(p, &offset, &button, sizeof(button));
	packet_read_int(p, &offset, &action, sizeof(action));
	packet_read_int(p, &offset, &mode, sizeof(mode));
	packet_read_slot(p, &offset, &slot_data);

	if (window == WINDOW_INVENTORY)
	{
		struct item_stack *stack = NULL;

		if (slot == -999)
			; // Outside of the window
		else if (slot < INVENTORY_CRAFT_OUTPUT && slot > INVENTORY_HOTBAR_8)
			return ERROR_NOT_ALLOWED;
		else
		{
			bedrock_assert(slot > 0 && slot < INVENTORY_SIZE, return ERROR_NOT_ALLOWED);
			stack = &client->inventory[slot];
		}

		// I clicked a valid slot with items in it
		if (stack != NULL && stack->count)
		{
			if (stack->id != slot_data.id || stack->count != slot_data.count) // XXX I have no metadata tracking?
				return ERROR_UNEXPECTED; // This is a lie

			// If I am already dragging an item replace it with this slot completely, even if I right clicked this slot.
			if (client->drag_data.stack.id)
			{
				/* Except if the IDs match */
				if (stack->id == client->drag_data.stack.id)
				{
					/* On right click take one item, else combine them as much as possible */
					if (button == BUTTON_RIGHT)
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
					/* else unknown button */
				}
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
				else
				{
					client->drag_data.stack.count = stack->count;
					bedrock_log(LEVEL_DEBUG, "click window: %s clicks on slot %d which contains %s and takes all %d blocks", client->name, slot, item_find_or_create(stack->id)->name, stack->count);

					/* Slot is now empty */
					stack->id = 0;
					stack->count = 0;
					stack->metadata = 0;
				}
			}
		}
		else
		{
			if (stack == NULL)
			{
				if (mode == OP_PAINT)
				{
					if (button != BUTTON_LEFT_START && button != BUTTON_RIGHT_START && button != BUTTON_LEFT_END && button != BUTTON_RIGHT_END)
						return ERROR_UNEXPECTED;
					/* Trying to (stop) paint with nothing held? */
					else if (!client->drag_data.stack.id)
						return ERROR_UNEXPECTED;

					if (button == BUTTON_LEFT_START || button == BUTTON_RIGHT_START)
					{
						/* Starting a paint while painting? */
						if (client->drag_data.data)
							return ERROR_NOT_ALLOWED;

						client->drag_data.data = button;

						bedrock_log(LEVEL_DEBUG, "click window: %s starts painting with %s using %s click", client->name, item_find_or_create(client->drag_data.stack.id)->name, button == BUTTON_LEFT_START ? "left" : "right");
					}
					else if (button == BUTTON_LEFT_END || button == BUTTON_RIGHT_END)
					{
						/* Ending a paint without starting? */
						if (!client->drag_data.data)
							return ERROR_NOT_ALLOWED;
						else if (client->drag_data.data == BUTTON_LEFT_START && button != BUTTON_LEFT_END)
							return ERROR_NOT_ALLOWED;
						else if (client->drag_data.data == BUTTON_RIGHT_START && button != BUTTON_RIGHT_END)
							return ERROR_NOT_ALLOWED;

						bedrock_log(LEVEL_DEBUG, "click window: %s stops painting with %s using %s click", client->name, item_find_or_create(client->drag_data.stack.id)->name, button == BUTTON_LEFT_END ? "left" : "right");

						client->drag_data.data = 0;

						/* Zero drag data if we don't have any items left, otherwise keep holding the items */
						if (!client->drag_data.stack.count)
						{
							client->drag_data.stack.id = 0;
							client->drag_data.stack.metadata = 0;
						}
					}
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
			}
			// Clicked an empty slot, might be placing blocks there, if we are currently dragging something
			else if (client->drag_data.stack.id)
			{
				/* Client is painting */
				if (mode == OP_PAINT || client->drag_data.data)
				{
					/* If client isn't actually painting kick them */
					if (!client->drag_data.data)
						return ERROR_NOT_ALLOWED;
					/* Or if they are painting but their mode isn't set to paint */
					else if (mode != OP_PAINT)
						return ERROR_NOT_ALLOWED;

					if (client->drag_data.data == BUTTON_RIGHT_START)
					{
						if (client->drag_data.stack.count)
						{
							bedrock_log(LEVEL_DEBUG, "click window: %s paints 1 block of %s in to empty slot %d", client->name, item_find_or_create(client->drag_data.stack.id)->name, slot);

							--client->drag_data.stack.count;
							stack->count = 1;
							stack->id = client->drag_data.stack.id;
						}
					}
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
			/* else clicked an empty slot but not dragging anything */
		}

		packet_send_confirm_transaction(client, window, action, true);
	}

	return offset;
}
