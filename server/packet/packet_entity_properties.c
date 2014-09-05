#include "server/client.h"
#include "server/packet.h"

void packet_send_entity_properties(struct client *client, const char *property, double value)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_ENTITY_PROPERTIES);

	packet_pack_varint(&packet, client->id);
	packet_pack_int(&packet, 1);
	packet_pack_string(&packet, property);
	packet_pack_double(&packet, value);
	packet_pack_varint(&packet, 0);

	client_send_packet(client, &packet);
}

