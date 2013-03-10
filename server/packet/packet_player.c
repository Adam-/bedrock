#include "server/client.h"
#include "server/packet.h"

int packet_player(struct client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint8_t on_ground;

	packet_read_int(p, &offset, &on_ground, sizeof(on_ground));

	client_update_position(client, *client_get_pos_x(client), *client_get_pos_y(client), *client_get_pos_z(client), *client_get_yaw(client), *client_get_pitch(client), client->stance, on_ground);

	return offset;
}
