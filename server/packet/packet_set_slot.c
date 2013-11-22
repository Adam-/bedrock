#include "server/client.h"
#include "server/packet.h"
#include "blocks/items.h"

void packet_send_set_slot(struct client *client, uint8_t window_id, uint16_t slot, struct item *item, uint8_t count, int16_t damage)
{
	bedrock_packet packet;
	struct item_stack slot_data;

	slot_data.id = item != NULL ? item->id : -1;
	slot_data.count = count;
	slot_data.metadata = damage;

	packet_init(&packet, SERVER_SET_SLOT);

	packet_pack_int(&packet, &window_id, sizeof(window_id));
	packet_pack_int(&packet, &slot, sizeof(slot));
	packet_pack_slot(&packet, &slot_data);

	client_send_packet(client, &packet);
}
