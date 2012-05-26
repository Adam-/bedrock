#include "server/client.h"
#include "packet/packet.h"

int packet_keep_alive(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint32_t id;

	packet_read_int(buffer, len, &offset, &id, sizeof(id));

	if (id == 0)
	{
		client_send_header(client, KEEP_ALIVE);
		client_send_int(client, &id, sizeof(id));
	}

	return offset;
}

void packet_send_keep_alive(struct bedrock_client *client, uint32_t id)
{
	client_send_header(client, KEEP_ALIVE);
	client_send_int(client, &id, sizeof(id));
}
