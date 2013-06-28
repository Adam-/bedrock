#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
#include "packet/packet_entity_equipment.h"
#include "nbt/nbt.h"
#include "util/list.h"

int packet_held_item_change(struct client *client, const bedrock_packet *p)
{
	int offset = PACKET_HEADER_LENGTH;
	bedrock_node *node;
	struct item_stack *stack;
	struct item *item;

	packet_read_int(p, &offset, &client->selected_slot, sizeof(client->selected_slot));

	if (client->selected_slot < 0 || client->selected_slot >= INVENTORY_HOTBAR_SIZE)
		return ERROR_UNEXPECTED;

	stack = &client->inventory[INVENTORY_HOTBAR_START + client->selected_slot];
	if (stack->count)
		item = item_find_or_create(stack->id);
	else
		item = item_find_or_create(-1);

	bedrock_log(LEVEL_DEBUG, "held item change: %s changes held item to slot %d, which contains %s", client->name, client->selected_slot, item->name);

	// No more digging now
	memset(&client->digging_data, 0, sizeof(client->digging_data));

	if (client->column != NULL)
		LIST_FOREACH(&client->column->players, node)
		{
			struct client *c = node->data;

			if (client == c)
				continue;

			packet_send_entity_equipment(c, client, ENTITY_EQUIPMENT_HELD, item, stack != NULL ? stack->metadata : 0);
		}

	return offset;
}
