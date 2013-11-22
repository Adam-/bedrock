#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_disconnect.h"

int packet_position_and_look(struct client *client, bedrock_packet *p)
{
	double x, y, stance, z;
	float yaw, pitch;
	uint8_t on_ground;

	packet_read_int(p, &x, sizeof(x));
	packet_read_int(p, &y, sizeof(y));
	packet_read_int(p, &stance, sizeof(stance));
	packet_read_int(p, &z, sizeof(z));
	packet_read_int(p, &yaw, sizeof(yaw));
	packet_read_int(p, &pitch, sizeof(pitch));
	packet_read_int(p, &on_ground, sizeof(on_ground));

	if (p->error)
		return p->error;

	if (!(client->state & STATE_BURSTING) && (abs(x - client->x) > 100 || abs(z - client->z) > 100))
	{
		packet_send_disconnect(client, "Moving too fast");
		return ERROR_OK;
	}

	client_update_position(client, x, y, z, yaw, pitch, stance, on_ground);

	return ERROR_OK;
}

void packet_send_position_and_look(struct client *client)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_PLAYER_POS_LOOK);

	packet_pack_int(&packet, &client->x, sizeof(client->x)); // X
	/*if (client->stance)
		packet_pack_int(&packet, &client->stance, sizeof(client->stance)); // Stance
	else
	{
		double d = client->y + 2;
		packet_pack_int(&packet, &d, sizeof(d)); // Stance
	}*/
	packet_pack_int(&packet, &client->y, sizeof(client->y)); // Y
	packet_pack_int(&packet, &client->z, sizeof(client->z)); // Z
	packet_pack_int(&packet, &client->yaw, sizeof(client->yaw)); // Yaw
	packet_pack_int(&packet, &client->pitch, sizeof(client->pitch)); // Pitch
	packet_pack_int(&packet, &client->on_ground, sizeof(client->on_ground)); // On ground

	client_send_packet(client, &packet);
}

