#include "server/client.h"
#include "server/packet.h"

void packet_send_block_change(struct client *client, int32_t x, uint8_t y, int32_t z, uint16_t id, uint8_t data)
{
	bedrock_packet packet;
	struct position pos = {
		.x = x,
		.y = y,
		.z = z
	};

	packet_init(&packet, SERVER_BLOCK_CHANGE);

	packet_pack_position(&packet, &pos);
	packet_pack_varint(&packet, id);

	client_send_packet(client, &packet);
}
