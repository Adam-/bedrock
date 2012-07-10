#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_disconnect.h"

int packet_position_and_look(struct bedrock_client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
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

	if (abs(x - *client_get_pos_x(client)) > 100 || abs(z - *client_get_pos_z(client)) > 100)
	{
		packet_send_disconnect(client, "Moving too fast");
		return offset;
	}

	client_update_position(client, x, y, z, yaw, pitch, stance, on_ground);

	return offset;
}

void packet_send_position_and_look(struct bedrock_client *client)
{
	bedrock_packet packet;

	packet_init(&packet, PLAYER_POS_LOOK);

	packet_pack_header(&packet, PLAYER_POS_LOOK);
	packet_pack_int(&packet, client_get_pos_x(client), sizeof(double)); // X
	if (client->stance)
		packet_pack_int(&packet, &client->stance, sizeof(client->stance)); // Stance
	else
	{
		double d = *client_get_pos_y(client) + 2;
		packet_pack_int(&packet, &d, sizeof(d)); // Stance
	}
	packet_pack_int(&packet, client_get_pos_y(client), sizeof(double)); // Y
	packet_pack_int(&packet, client_get_pos_z(client), sizeof(double)); // Z
	packet_pack_int(&packet, client_get_yaw(client), sizeof(float)); // Yaw
	packet_pack_int(&packet, client_get_pitch(client), sizeof(float)); // Pitch
	packet_pack_int(&packet, client_get_on_ground(client), sizeof(uint8_t)); // On ground

	client_send_packet(client, &packet);
}

