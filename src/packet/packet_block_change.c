#include "server/client.h"
#include "server/packet.h"

void packet_send_block_change(struct bedrock_client *client, int32_t x, uint8_t y, int32_t z, uint8_t id, uint8_t data)
{
	bedrock_packet packet;

	packet_init(&packet, BLOCK_CHANGE);

	packet_pack_header(&packet, BLOCK_CHANGE);
	packet_pack_int(&packet, &x, sizeof(x));
	packet_pack_int(&packet, &y, sizeof(y));
	packet_pack_int(&packet, &z, sizeof(z));
	packet_pack_int(&packet, &id, sizeof(id));
	packet_pack_int(&packet, &data, sizeof(data));

	client_send_packet(client, &packet);
}
