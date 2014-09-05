#include "server/client.h"
#include "nbt/nbt.h"

int packet_creative_inventory_action(struct client *client, bedrock_packet *packet)
{
	int16_t id;
	struct item_stack slot, *stack;
	struct item *item;

	packet_read_short(packet, &id);
	packet_read_slot(packet, &slot);

	if (packet->error)
		return packet->error;

	if (client->gamemode != GAMEMODE_CREATIVE)
		return ERROR_UNEXPECTED;

	if (id < -1 || id >= INVENTORY_SIZE)
		return ERROR_NOT_ALLOWED;

	if (slot.id == -1)
	{
		// what is this?
		bedrock_log(LEVEL_DEBUG, "creative inventory action: Negative slot for %s?", client->name);
		return ERROR_OK;
	}

	item = item_find_or_create(slot.id);
	if (item == NULL)
		return ERROR_UNEXPECTED;

	if (id == -1)
	{
		// Dropping item
		bedrock_log(LEVEL_DEBUG, "creative inventory action: %s drops %d %s", client->name, slot.count, item->name);

		return ERROR_OK;
	}

	bedrock_log(LEVEL_DEBUG, "creative inventory action: %s puts %d %s in to slot %d", client->name, slot.count, item->name, id);
	
	stack = &client->inventory[id];

	if (stack->id != ITEM_NONE && stack->id != slot.id)
	{
		client->drag_data.stack = *stack;
		bedrock_log(LEVEL_DEBUG, "creative inventory action: %s is now dragging %d %s", client->name, client->drag_data.stack.count, item_find_or_create(client->drag_data.stack.id)->name);
	}

	*stack = slot;

	return ERROR_OK;
}

