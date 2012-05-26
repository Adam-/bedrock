#include "server/client.h"
#include "packet/packet.h"

void packet_send_set_slot(struct bedrock_client *client, uint8_t window_id, uint16_t slot, int16_t item, uint8_t count, int16_t metadata)
{
	client_send_header(client, SET_SLOT);
	client_send_int(client, &window_id, sizeof(window_id));
	client_send_int(client, &slot, sizeof(slot));

	client_send_int(client, &item, sizeof(item));
	if (item != -1)
	{
		client_send_int(client, &count, sizeof(count));
		client_send_int(client, &metadata, sizeof(metadata));
		// Enchantable items etc require more here.
	}
}
