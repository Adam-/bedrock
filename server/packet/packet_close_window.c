#include "server/client.h"
#include "server/packet.h"

int packet_close_window(struct client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint8_t window;

	packet_read_int(p, &offset, &window, sizeof(window));

	if (client->window != window)
		return ERROR_UNEXPECTED;

	client->window = 0;

	return offset;
}
