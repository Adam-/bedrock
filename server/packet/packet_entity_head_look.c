#include "server/client.h"
#include "server/packet.h"

void packet_send_entity_head_look(struct client *client, struct client *target)
{
	bedrock_packet packet;
	int8_t new_y = (target->yaw / 360.0) * 256;

	packet_init(&packet, SERVER_ENTITY_HEAD_LOOK);

	packet_pack_varint(&packet, target->id);
	packet_pack_byte(&packet, new_y);

	client_send_packet(client, &packet);
}
