#include "server/client.h"
#include "server/packet.h"

int packet_ping(struct client *client, bedrock_packet *p)
{
	int64_t l;
	bedrock_packet packet;

	packet_read_long(p, &l);

	if (p->error)
		return p->error;

	packet_init(&packet, STATUS_SERVER_PING);
	packet_pack_long(&packet, l);
	client_send_packet(client, &packet);

	return ERROR_OK;
}

