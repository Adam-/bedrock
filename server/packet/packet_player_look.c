#include "server/client.h"
#include "server/packet.h"

int packet_player_look(struct client *client, bedrock_packet *p)
{
	float yaw, pitch;
	bool on_ground;

	packet_read_float(p, &yaw);
	packet_read_float(p, &pitch);
	packet_read_bool(p, &on_ground);

	if (p->error)
		return p->error;

	client_update_position(client, client->x, client->y, client->z, yaw, pitch, client->stance, on_ground);

	return ERROR_OK;
}
