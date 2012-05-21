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

	client_set_yaw(client, yaw);
	client_set_pitch(client, pitch);
	client_set_on_ground(client, on_ground);

	client_update_position(client);

	return offset;
}
