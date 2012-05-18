#include "server/client.h"
#include "packet/packet.h"

int packet_position_and_look(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	double x, y, stance, z;
	float yaw, pitch;
	uint8_t on_ground;

	packet_read_int(buffer, len, &offset, &x, sizeof(x));
	packet_read_int(buffer, len, &offset, &y, sizeof(y));
	packet_read_int(buffer, len, &offset, &stance, sizeof(stance));
	packet_read_int(buffer, len, &offset, &z, sizeof(z));
	packet_read_int(buffer, len, &offset, &yaw, sizeof(yaw));
	packet_read_int(buffer, len, &offset, &pitch, sizeof(pitch));
	packet_read_int(buffer, len, &offset, &on_ground, sizeof(on_ground));

	return offset;
}

void packet_send_player_and_look(struct bedrock_client *client)
{
	double d;

	client_send_header(client, PLAYER_POS_LOOK);
	client_send_int(client, client_get_pos_x(client), sizeof(double)); // X
	d = *client_get_pos_y(client);
	d += 2;
	client_send_int(client, &d, sizeof(d)); // Stance
	client_send_int(client, &d, sizeof(d)); // Y
	client_send_int(client, client_get_pos_z(client), sizeof(double)); // Z
	client_send_int(client, client_get_yaw(client), sizeof(float)); // Yaw
	client_send_int(client, client_get_pitch(client), sizeof(float)); // Pitch
	client_send_int(client, client_get_on_ground(client), sizeof(uint8_t)); // On ground
}

