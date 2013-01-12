#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
#include "packet/packet_entity_equipment.h"
#include "nbt/nbt.h"
#include "util/list.h"

int packet_held_item_change(struct client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	bedrock_node *node;
	nbt_tag *tag;
	struct item *item;
	uint16_t *damage;

	packet_read_int(p, &offset, &client->selected_slot, sizeof(client->selected_slot));

	tag = client_get_inventory_tag(client, client->selected_slot);
	if (tag != NULL)
	{
		uint16_t *id = nbt_read(tag, TAG_SHORT, 1, "id");
		item = item_find_or_create(*id);
		damage = nbt_read(tag, TAG_SHORT, 1, "Damage");
	}
	else
	{
		item = item_find_or_create(-1);
		damage = NULL;
	}

	// No more digging now
	memset(&client->digging_data, 0, sizeof(client->digging_data));

	if (client->column != NULL)
		LIST_FOREACH(&client->column->players, node)
		{
			struct client *c = node->data;

			if (client == c)
				continue;

			packet_send_entity_equipment(c, client, ENTITY_EQUIPMENT_HELD, item, damage != NULL ? *damage : 0);
		}

	return offset;
}
