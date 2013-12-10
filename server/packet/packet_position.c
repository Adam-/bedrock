#include "server/client.h"
#include "server/packet.h"
#include "server/packets.h"

int packet_position(struct client *client, bedrock_packet *p)
{
	double x, y, stance, z;
	uint8_t on_ground;

	packet_read_int(p, &x, sizeof(x));
	packet_read_int(p, &y, sizeof(y));
	packet_read_int(p, &stance, sizeof(stance));
	packet_read_int(p, &z, sizeof(z));
	packet_read_int(p, &on_ground, sizeof(on_ground));

	if (p->error)
		return p->error;

	if (!(client->state & STATE_BURSTING) && (abs(x - client->x) > 100 || abs(z - client->z) > 100))
	{
		packet_send_disconnect(client, "Moving too fast");
		return ERROR_OK;
	}

	client_update_position(client, x, y, z, client->yaw, client->pitch, stance, on_ground);

	return ERROR_OK;
}
