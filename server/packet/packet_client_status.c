#include "server/client.h"
#include "server/packet.h"

int packet_client_status(struct client bedrock_attribute_unused *client, bedrock_packet *p)
{
	int8_t b;

	packet_read_byte(p, &b);

	if (p->error)
		return p->error;

	return ERROR_OK;
}

