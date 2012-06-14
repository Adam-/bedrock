#include "server/client.h"
#include "server/packet.h"
#include "blocks/items.h"

void packet_send_set_slot(struct bedrock_client *client, uint8_t window_id, uint16_t slot, struct bedrock_item *item, uint8_t count, int16_t damage)
{
	bedrock_packet packet;

	packet_init(&packet, SET_SLOT);

	packet_pack_header(&packet, SET_SLOT);
	packet_pack_int(&packet, &window_id, sizeof(window_id));
	packet_pack_int(&packet, &slot, sizeof(slot));

	if (item == NULL)
	{
		int16_t s = -1;
		packet_pack_int(&packet, &s, sizeof(s));
	}
	else
	{
		packet_pack_int(&packet, &item->id, sizeof(item->id));
		packet_pack_int(&packet, &count, sizeof(count));
		packet_pack_int(&packet, &damage, sizeof(damage));
		if (item->flags & ITEM_FLAG_DAMAGABLE)
		{
			int16_t s = -1;
			packet_pack_int(&packet, &s, sizeof(s));
		}
	}

	client_send_packet(client, &packet);
}
