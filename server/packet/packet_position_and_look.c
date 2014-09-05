#include "server/client.h"
#include "server/packet.h"
#include "server/packets.h"

int packet_position_and_look(struct client *client, bedrock_packet *p)
{
	double x, y, z;
	float yaw, pitch;
	bool on_ground;

	packet_read_double(p, &x);
	packet_read_double(p, &y);
	packet_read_double(p, &z);
	packet_read_float(p, &yaw);
	packet_read_float(p, &pitch);
	packet_read_bool(p, &on_ground);

	if (p->error)
		return p->error;

	if (!(client->state & STATE_BURSTING) && (abs(x - client->x) > 100 || abs(z - client->z) > 100))
	{
		packet_send_disconnect(client, "Moving too fast");
		return ERROR_OK;
	}

	client_update_position(client, x, y, z, yaw, pitch, y + 1.62, on_ground); // XXX?

	return ERROR_OK;
}

void packet_send_position_and_look(struct client *client)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_PLAYER_POS_LOOK);

	packet_pack_double(&packet, client->x);
	packet_pack_double(&packet, client->y + BEDROCK_PLAYER_HEIGHT);
	packet_pack_double(&packet, client->z);
	packet_pack_float(&packet, client->yaw);
	packet_pack_float(&packet, client->pitch);
	packet_pack_byte(&packet, 0);

	client_send_packet(client, &packet);
}

