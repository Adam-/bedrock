#include "server/bedrock.h"
#include "server/client.h"
#include "server/column.h"
#include "server/packet.h"
#include "packet/packet_set_slot.h" // XXX for WINDOW_
#include "packet/packet_confirm_transaction.h"
#include "nbt/nbt.h"

#include <math.h>

int packet_click_window(struct client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint8_t window;
	int16_t slot;
	uint8_t right_click;
	uint16_t action;
	uint8_t shift;
	struct item_stack slot_data;

	packet_read_int(p, &offset, &window, sizeof(window));
	packet_read_int(p, &offset, &slot, sizeof(slot));
	packet_read_int(p, &offset, &right_click, sizeof(right_click));
	packet_read_int(p, &offset, &action, sizeof(action));
	packet_read_int(p, &offset, &shift, sizeof(shift));
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
			if (client->window_drag_data.id)
			{
				// However if I am right clicking and this slot is of the same type as my drag type, move one item
				if (right_click && stack->id == client->window_drag_data.id)
				{
					if (stack->count < BEDROCK_MAX_ITEMS_PER_STACK && client->window_drag_data.count)
					{
						++stack->count;
						--client->window_drag_data.count;

						bedrock_log(LEVEL_DEBUG, "click window: %s moves one item from stack of %s to %d, has %d left", client->name, item_find_or_create(client->window_drag_data.id)->name, slot, client->window_drag_data.count);

						// Might have been the last item, zero out drag state
						if (client->window_drag_data.count == 0)
						{
							client->window_drag_data.id = 0;
							client->window_drag_data.metadata = 0;
						}
					}
				}
				// Replacing a slot
				else
				{
					/* Swap the drag data with this slot */
					stack->id = client->window_drag_data.id;
					stack->count = client->window_drag_data.count;
					stack->metadata = client->window_drag_data.metadata;

					/* We are now dragging what was in this slot */
					client->window_drag_data.id = slot_data.id;
					client->window_drag_data.count = slot_data.count;
					client->window_drag_data.metadata = slot_data.metadata;

					bedrock_log(LEVEL_DEBUG, "click window: %s replaces slot %d with %d blocks of %s", client->name, slot, client->window_drag_data.count, item_find_or_create(client->window_drag_data.id)->name);
				}
			}
			else
			{
				/* This item is now being dragged and is no longer in its slot */
				client->window_drag_data.id = stack->id;
				client->window_drag_data.metadata = stack->metadata;

				if (right_click)
				{
					/* We only want half of the blocks here. If there is an odd count then they are holding the larger. */
					double dcount = (double) stack->count / 2;
					bool even = floor(dcount) == dcount;

					// If this isn't an even outcome give the client one more
					if (even == false)
						++dcount;

					client->window_drag_data.count = dcount;
					bedrock_log(LEVEL_DEBUG, "click window: %s right clicks on slot %d which contains %s and takes %d", client->name, slot, item_find_or_create(stack->id)->name, client->window_drag_data.count);

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
					client->window_drag_data.count = stack->count;
					bedrock_log(LEVEL_DEBUG, "click window: %s clicks on slot %d which contains %s and takes all blocks", client->name, slot, item_find_or_create(stack->id)->name);

					/* Slot is now empty */
					stack->id = 0;
					stack->count = 0;
					stack->metadata = 0;
				}
			}
		}
		else
		{
			// Dropping items on the ground
			if (stack == NULL)
			{
				if (client->window_drag_data.id)
				{
					/* Spawn dropped item */
					struct dropped_item *di = bedrock_malloc(sizeof(struct dropped_item));
					struct column *col;

					di->item = item_find_or_create(client->window_drag_data.id);
					di->count = client->window_drag_data.count;
					di->data = client->window_drag_data.metadata;
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

					bedrock_log(LEVEL_DEBUG, "click window: %s drops %d blocks of %s", client->name, client->window_drag_data.count, item_find_or_create(client->window_drag_data.id)->name);
				}

				client->window_drag_data.id = 0;
				client->window_drag_data.count = 0;
				client->window_drag_data.metadata = 0;
			}
			// Clicked an empty slot, might be placing blocks there, if we are currently dragging something
			else if (client->window_drag_data.id)
			{
				/* Replace slot with our drag data */
				stack->id = client->window_drag_data.id;
				stack->metadata = client->window_drag_data.metadata;

				// On a right click we only transfer one item, otherwise take them all
				if (right_click && client->window_drag_data.count)
				{
					stack->count = 1;
					--client->window_drag_data.count;

					bedrock_log(LEVEL_DEBUG, "click window: %s right click places 1 block of %s in empty slot %d", client->name, item_find_or_create(client->window_drag_data.id)->name, slot);
				}
				else
				{
					stack->count = client->window_drag_data.count;
					bedrock_log(LEVEL_DEBUG, "click window: %s places %d blocks of %s in empty slot %d", client->name, client->window_drag_data.count, item_find_or_create(client->window_drag_data.id)->name, slot);
				}

				// Zero out drag state if this wasn't a right click replace or if there are no items left
				if (right_click == false || client->window_drag_data.count == 0)
				{
					client->window_drag_data.id = 0;
					client->window_drag_data.count = 0;
					client->window_drag_data.metadata = 0;
				}
			}
		}

		packet_send_confirm_transaction(client, window, action, true);
	}

	return offset;
}
