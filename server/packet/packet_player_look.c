#include "server/client.h"
#include "server/packet.h"

int packet_player_look(struct client *client, const bedrock_packet *p)
{
	int offset = PACKET_HEADER_LENGTH;
	float yaw, pitch;
	uint8_t on_ground;

	packet_read_int(p, &offset, &yaw, sizeof(yaw));
	packet_read_int(p, &offset, &pitch, sizeof(pitch));
	packet_read_int(p, &offset, &on_ground, sizeof(on_ground));

	client_update_position(client, client->x, client->y, client->z, yaw, pitch, client->stance, on_ground);

	return offset;
}
