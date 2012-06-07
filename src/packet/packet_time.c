#include "server/client.h"
#include "server/packet.h"

void packet_send_time(struct bedrock_client *client)
{
	client_send_header(client, TIME);
	client_send_int(client, &client->world->time, sizeof(client->world->time));
}
