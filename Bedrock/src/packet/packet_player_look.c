#include "server/client.h"
#include "packet/packet.h"

int packet_player_look(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	float yaw, pitch;
	uint8_t on_ground;

	packet_read_int(buffer, len, &offset, &yaw, sizeof(yaw));
	packet_read_int(buffer, len, &offset, &pitch, sizeof(pitch));
	packet_read_int(buffer, len, &offset, &on_ground, sizeof(on_ground));

	client_update_position(client, *client_get_pos_x(client), *client_get_pos_y(client), *client_get_pos_z(client), yaw, pitch, on_ground);

	return offset;
}
