#include "server/client.h"
#include "server/packet.h"

int packet_client_status(struct client bedrock_attribute_unused *client, bedrock_packet *p)
{
	uint8_t b;

	packet_read_int(p, &b, sizeof(b));

	if (p->error)
		return p->error;

	return ERROR_OK;
}

