#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_disconnect.h"

int packet_keep_alive(struct client *client, bedrock_packet *p)
{
	uint32_t id;

	packet_read_int(p, &id, sizeof(id));

	if (p->error)
		return p->error;
	else if (id == 0 || client->ping_id == 0)
		;
	else if (id != client->ping_id)
	{
		bedrock_log(LEVEL_INFO, "client: Dropping client %s (%s) due to invalid keepalive ID.", client->name, client_get_ip(client));
		packet_send_disconnect(client, "Ping timeout");
	}
	else
	{
		client->ping = bedrock_time - client->ping_time_sent;

		if (client->state & STATE_IN_GAME && client->state & STATE_BURSTING)
		{
			client->state &= ~STATE_BURSTING;
			bedrock_log(LEVEL_DEBUG, "client: End of burst for %s", client->name);
		}
	}

	return ERROR_OK;
}

void packet_send_keep_alive(struct client *client, uint32_t id)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_KEEP_ALIVE);

	packet_pack_int(&packet, &id, sizeof(id));

	client_send_packet(client, &packet);

	client->ping_id = id;
	client->ping_time_sent = bedrock_time;
}
