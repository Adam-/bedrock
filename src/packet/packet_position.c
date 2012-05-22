#include "server/client.h"
#include "packet/packet.h"

int packet_position(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	double x, y, stance, z;
	uint8_t on_ground;

	packet_read_int(buffer, len, &offset, &x, sizeof(x));
	packet_read_int(buffer, len, &offset, &y, sizeof(y));
	packet_read_int(buffer, len, &offset, &stance, sizeof(stance));
	packet_read_int(buffer, len, &offset, &z, sizeof(z));
	packet_read_int(buffer, len, &offset, &on_ground, sizeof(on_ground));

	client_update_position(client, x, y, z, *client_get_yaw(client), *client_get_pitch(client), on_ground);

	return offset;
}
