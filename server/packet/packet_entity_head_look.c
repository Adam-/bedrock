#include "server/client.h"
#include "server/packet.h"

void packet_send_entity_head_look(struct client *client, struct client *target)
{
	bedrock_packet packet;
	int8_t new_y = (target->yaw / 360.0) * 256;

	packet_init(&packet, ENTITY_HEAD_LOOK);

	packet_pack_header(&packet, ENTITY_HEAD_LOOK);
	packet_pack_int(&packet, &target->id, sizeof(target->id));
	packet_pack_int(&packet, &new_y, sizeof(new_y));

	client_send_packet(client, &packet);
}
