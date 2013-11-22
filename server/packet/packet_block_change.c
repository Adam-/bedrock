#include "server/client.h"
#include "server/packet.h"

void packet_send_block_change(struct client *client, int32_t x, uint8_t y, int32_t z, uint16_t id, uint8_t data)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_BLOCK_CHANGE);

	packet_pack_int(&packet, &x, sizeof(x));
	packet_pack_int(&packet, &y, sizeof(y));
	packet_pack_int(&packet, &z, sizeof(z));
	packet_pack_varint(&packet, id);
	packet_pack_int(&packet, &data, sizeof(data));

	client_send_packet(client, &packet);
}
