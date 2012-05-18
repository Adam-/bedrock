#include "server/client.h"
#include "packet/packet.h"

int packet_send_spawn_point(struct bedrock_client *client, int32_t x, int32_t y, int32_t z)
{
	client_send_header(client, SPAWN_POINT);
	client_send_int(client, &x, sizeof(x));
	client_send_int(client, &y, sizeof(y));
	client_send_int(client, &z, sizeof(z));
}
