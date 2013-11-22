#include "server/client.h"
#include "server/packet.h"

void packet_send_time(struct client *client)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_TIME);

	packet_pack_int(&packet, &client->world->creation, sizeof(client->world->creation));
	packet_pack_int(&packet, &client->world->time, sizeof(client->world->time));

	client_send_packet(client, &packet);
}
