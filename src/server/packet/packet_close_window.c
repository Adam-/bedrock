#include "server/client.h"
#include "server/packet.h"

int packet_close_window(struct bedrock_client bedrock_attribute_unused *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint8_t window;

	packet_read_int(p, &offset, &window, sizeof(window));

	return offset;
}
