#include "server/client.h"
#include "server/packet.h"

int packet_position_and_look(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = PACKET_HEADER_LENGTH;
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

	client_update_position(client, x, y, z, yaw, pitch, stance, on_ground);

	return offset;
}

void packet_send_position_and_look(struct bedrock_client *client)
{
	client_send_header(client, PLAYER_POS_LOOK);
	client_send_int(client, client_get_pos_x(client), sizeof(double)); // X
	if (client->stance)
		client_send_int(client, &client->stance, sizeof(client->stance)); // Stance
	else
	{
		double d = *client_get_pos_y(client) + 2;
		client_send_int(client, &d, sizeof(d)); // Stance
	}
	client_send_int(client, client_get_pos_y(client), sizeof(double)); // Y
	client_send_int(client, client_get_pos_z(client), sizeof(double)); // Z
	client_send_int(client, client_get_yaw(client), sizeof(float)); // Yaw
	client_send_int(client, client_get_pitch(client), sizeof(float)); // Pitch
	client_send_int(client, client_get_on_ground(client), sizeof(uint8_t)); // On ground
}

