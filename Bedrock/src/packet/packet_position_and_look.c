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

	client_update_position(client, x, y, z, yaw, pitch, on_ground);

	return offset;
}

void packet_send_position_and_look(struct bedrock_client *client, struct bedrock_client *c)
{
	double d;

	client_send_header(client, PLAYER_POS_LOOK);
	client_send_int(client, client_get_pos_x(c), sizeof(double)); // X
	d = *client_get_pos_y(c);
	d += 2;
	client_send_int(client, &d, sizeof(d)); // Stance
	client_send_int(client, &d, sizeof(d)); // Y
	client_send_int(client, client_get_pos_z(c), sizeof(double)); // Z
	client_send_int(client, client_get_yaw(c), sizeof(float)); // Yaw
	client_send_int(client, client_get_pitch(c), sizeof(float)); // Pitch
	client_send_int(client, client_get_on_ground(c), sizeof(uint8_t)); // On ground
}

