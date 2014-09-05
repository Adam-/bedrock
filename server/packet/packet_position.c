#include "server/client.h"
#include "server/packet.h"
#include "server/packets.h"

int packet_position(struct client *client, bedrock_packet *p)
{
	double x, y, z;
	bool on_ground;

	packet_read_double(p, &x);
	packet_read_double(p, &y);
	packet_read_double(p, &z);
	packet_read_bool(p, &on_ground);

	if (p->error)
		return p->error;

	if (!(client->state & STATE_BURSTING) && (abs(x - client->x) > 100 || abs(z - client->z) > 100))
	{
		packet_send_disconnect(client, "Moving too fast");
		return ERROR_OK;
	}

	client_update_position(client, x, y, z, client->yaw, client->pitch, y + 1.62, on_ground); // XXX?

	return ERROR_OK;
}
