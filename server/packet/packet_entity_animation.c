#include "server/client.h"
#include "server/packet.h"
#include "server/packets.h"
#include "server/column.h"

int packet_entity_animation(struct client *client, bedrock_packet *p)
{
	return ERROR_OK;
}

void packet_send_entity_animation(struct client *client, struct client *target, uint8_t anim)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_ANIMATION);

	packet_pack_varint(&packet, target->id);
	packet_pack_byte(&packet, anim);

	client_send_packet(client, &packet);
}
