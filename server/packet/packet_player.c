#include "server/client.h"
#include "server/packet.h"

int packet_player(struct client *client, bedrock_packet *p)
{
	bool on_ground;

	packet_read_bool(p, &on_ground);

	if (p->error)
		return p->error;

	client_update_position(client, client->x, client->y, client->z, client->yaw, client->pitch, client->stance, on_ground);

	return ERROR_OK;
}
