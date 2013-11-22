#include "server/client.h"
#include "server/packet.h"

void packet_send_entity_properties(struct client *client, const char *property, double value)
{
	bedrock_packet packet;
	int32_t count = 1;
	int16_t list_count = 0;

	packet_init(&packet, SERVER_ENTITY_PROPERTIES);

	packet_pack_int(&packet, &client->id, sizeof(client->id));
	packet_pack_int(&packet, &count, sizeof(count));
	packet_pack_string(&packet, property);
	packet_pack_int(&packet, &value, sizeof(value));
	packet_pack_int(&packet, &list_count, sizeof(list_count));

	client_send_packet(client, &packet);
}

