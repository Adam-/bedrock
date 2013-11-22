#include "server/client.h"
#include "server/packet.h"

int packet_player_look(struct client *client, bedrock_packet *p)
{
	float yaw, pitch;
	uint8_t on_ground;

	packet_read_int(p, &yaw, sizeof(yaw));
	packet_read_int(p, &pitch, sizeof(pitch));
	packet_read_int(p, &on_ground, sizeof(on_ground));

	if (p->error)
		return p->error;

	client_update_position(client, client->x, client->y, client->z, yaw, pitch, client->stance, on_ground);

	return ERROR_OK;
}
