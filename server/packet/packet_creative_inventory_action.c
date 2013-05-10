#include "server/client.h"
#include "nbt/nbt.h"
#include "packet/packet_change_game_state.h"

int packet_creative_inventory_action(struct client *client, const bedrock_packet *packet)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint16_t id;
	struct item_stack slot, *stack;
	int opt;

	packet_read_int(packet, &offset, &id, sizeof(id));
	packet_read_slot(packet, &offset, &slot);

	if (offset <= ERROR_UNKNOWN)
		return offset;

	nbt_copy(client->data, TAG_INT, &opt, sizeof(opt), 1, "playerGameType");
	if (opt != GAMEMODE_CREATIVE)
		return ERROR_UNEXPECTED;

	if (id >= INVENTORY_SIZE)
		return ERROR_NOT_ALLOWED;

	bedrock_log(LEVEL_DEBUG, "creative inventory action: %s puts %d %s in to slot %d", client->name, slot.count, item_find_or_create(slot.id)->name, id);
	
	stack = &client->inventory[id];

	if (stack->id != ITEM_NONE && stack->id != slot.id)
	{
		client->drag_data.stack = *stack;
		bedrock_log(LEVEL_DEBUG, "creative inventory action: %s is now dragging %d %s", client->name, client->drag_data.stack.count, item_find_or_create(client->drag_data.stack.id)->name);
	}

	*stack = slot;

	return offset;
}

