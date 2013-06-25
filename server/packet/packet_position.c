#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_disconnect.h"

int packet_position(struct client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	double x, y, stance, z;
	uint8_t on_ground;

	packet_read_int(p, &offset, &x, sizeof(x));
	packet_read_int(p, &offset, &y, sizeof(y));
	packet_read_int(p, &offset, &stance, sizeof(stance));
	packet_read_int(p, &offset, &z, sizeof(z));
	packet_read_int(p, &offset, &on_ground, sizeof(on_ground));

	if (!(client->authenticated & STATE_BURSTING) && (abs(x - client->x) > 100 || abs(z - client->z) > 100))
	{
		packet_send_disconnect(client, "Moving too fast");
		return offset;
	}

	client_update_position(client, x, y, z, client->yaw, client->pitch, stance, on_ground);

	return offset;
}
