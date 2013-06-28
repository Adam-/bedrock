#include "server/client.h"
#include "server/packet.h"

int packet_close_window(struct client *client, const bedrock_packet *p)
{
	int offset = PACKET_HEADER_LENGTH;
	uint8_t window;

	packet_read_int(p, &offset, &window, sizeof(window));

	if (client->window_data.id != window)
		return ERROR_UNEXPECTED;

	memset(&client->window_data, 0, sizeof(client->window_data));

	return offset;
}
