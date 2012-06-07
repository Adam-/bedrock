#include "server/client.h"
#include "server/packet.h"

int packet_close_window(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint8_t window;

	packet_read_int(buffer, len, &offset, &window, sizeof(window));

	return offset;
}
