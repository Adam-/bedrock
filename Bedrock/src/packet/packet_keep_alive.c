#include "server/client.h"
#include "packet/packet.h"

int packet_keep_alive(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	uint32_t id;

	packet_read_int(buffer, len, &offset, &id, sizeof(id));

	client_send_header(client, KEEP_ALIVE);
	client_send_int(client, &id, sizeof(id));

	return offset;
}
