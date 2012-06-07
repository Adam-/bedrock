#include "server/client.h"
#include "server/packet.h"

int packet_player_digging(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint8_t status;
	uint32_t x;
	uint8_t y;
	uint32_t z;
	uint8_t face;

	packet_read_int(buffer, len, &offset, &status, sizeof(status));
	packet_read_int(buffer, len, &offset, &x, sizeof(x));
	packet_read_int(buffer, len, &offset, &y, sizeof(y));
	packet_read_int(buffer, len, &offset, &z, sizeof(z));
	packet_read_int(buffer, len, &offset, &face, sizeof(face));

	return offset;
}
