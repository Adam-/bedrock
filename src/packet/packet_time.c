#include "server/client.h"
#include "server/packet.h"

void packet_send_time(struct bedrock_client *client)
{
	bedrock_packet packet;

	packet_init(&packet, HANDSHAKE);

	packet_pack_header(&packet, TIME);
	packet_pack_int(&packet, &client->world->time, sizeof(client->world->time));

	client_send_packet(client, &packet);
}
