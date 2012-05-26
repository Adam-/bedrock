#include "server/client.h"
#include "packet/packet.h"
#include "blocks/items.h"

void packet_send_set_slot(struct bedrock_client *client, uint8_t window_id, uint16_t slot, struct bedrock_item *item, uint8_t count, int16_t damage)
{
	client_send_header(client, SET_SLOT);
	client_send_int(client, &window_id, sizeof(window_id));
	client_send_int(client, &slot, sizeof(slot));

	if (item == NULL)
	{
		int16_t s = -1;
		client_send_int(client, &s, sizeof(s));
	}
	else
	{
		client_send_int(client, &item->id, sizeof(int16_t));
		client_send_int(client, &count, sizeof(count));
		client_send_int(client, &damage, sizeof(damage));
		if (item->flags & ITEM_FLAG_DAMAGABLE)
		{
			int16_t s = -1;
			client_send_int(client, &s, sizeof(s));
		}
	}
}
