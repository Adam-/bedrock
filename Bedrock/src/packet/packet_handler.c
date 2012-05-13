#include "packet/packet.h"

int packet_keep_alive(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	uint32_t id;
	packet_read_int(buffer, 1, &id, sizeof(id));

	bedrock_client_send(client, &id, sizeof(id));

	return 5;
}

int packet_login_request(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	return 0;
}
