#include "server/client.h"
#include "packet/packet.h"

int packet_player_look(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	float yaw, pitch;
	uint8_t bool;

	packet_read_int(buffer, len, &offset, &yaw, sizeof(yaw));
	packet_read_int(buffer, len, &offset, &pitch, sizeof(pitch));
	packet_read_int(buffer, len, &offset, &bool, sizeof(bool));

	return offset;
}
