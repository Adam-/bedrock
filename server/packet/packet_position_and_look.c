#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_disconnect.h"

int packet_position_and_look(struct client *client, const bedrock_packet *p)
{
	int offset = PACKET_HEADER_LENGTH;
	double x, y, stance, z;
	float yaw, pitch;
	uint8_t on_ground;

	packet_read_int(p, &offset, &x, sizeof(x));
	packet_read_int(p, &offset, &y, sizeof(y));
	packet_read_int(p, &offset, &stance, sizeof(stance));
	packet_read_int(p, &offset, &z, sizeof(z));
	packet_read_int(p, &offset, &yaw, sizeof(yaw));
	packet_read_int(p, &offset, &pitch, sizeof(pitch));
	packet_read_int(p, &offset, &on_ground, sizeof(on_ground));

	if (!(client->authenticated & STATE_BURSTING) && (abs(x - client->x) > 100 || abs(z - client->z) > 100))
	{
		packet_send_disconnect(client, "Moving too fast");
		return offset;
	}

	client_update_position(client, x, y, z, yaw, pitch, stance, on_ground);

	return offset;
}

void packet_send_position_and_look(struct client *client)
{
	bedrock_packet packet;

	packet_init(&packet, PLAYER_POS_LOOK);

	packet_pack_header(&packet, PLAYER_POS_LOOK);
	packet_pack_int(&packet, &client->x, sizeof(client->x)); // X
	if (client->stance)
		packet_pack_int(&packet, &client->stance, sizeof(client->stance)); // Stance
	else
	{
		double d = client->y + 2;
		packet_pack_int(&packet, &d, sizeof(d)); // Stance
	}
	packet_pack_int(&packet, &client->y, sizeof(client->y)); // Y
	packet_pack_int(&packet, &client->z, sizeof(client->z)); // Z
	packet_pack_int(&packet, &client->yaw, sizeof(client->yaw)); // Yaw
	packet_pack_int(&packet, &client->pitch, sizeof(client->pitch)); // Pitch
	packet_pack_int(&packet, &client->on_ground, sizeof(client->on_ground)); // On ground

	client_send_packet(client, &packet);
}

