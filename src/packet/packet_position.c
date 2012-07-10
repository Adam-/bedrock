#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_disconnect.h"

int packet_position(struct bedrock_client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	double x, y, stance, z;
	uint8_t on_ground;

	packet_read_int(p, &offset, &x, sizeof(x));
	packet_read_int(p, &offset, &y, sizeof(y));
	packet_read_int(p, &offset, &stance, sizeof(stance));
	packet_read_int(p, &offset, &z, sizeof(z));
	packet_read_int(p, &offset, &on_ground, sizeof(on_ground));

	if (abs(x - *client_get_pos_x(client)) > 100 || abs(z - *client_get_pos_z(client)) > 100)
	{
		packet_send_disconnect(client, "Moving too fast");
		return offset;
	}

	client_update_position(client, x, y, z, *client_get_yaw(client), *client_get_pitch(client), stance, on_ground);

	return offset;
}
