#include "server/client.h"

static uint8_t window_id = 0;

void packet_send_open_window(struct client *client, uint8_t type, const char *title, uint8_t slots)
{
	bedrock_packet packet;
	uint8_t use_title = 1;

	if (!title)
	{
		title = "";
		use_title = 0;
	}

	while (++window_id == 0);
	
	packet_init(&packet, OPEN_WINDOW);

	packet_pack_header(&packet, OPEN_WINDOW);
	packet_pack_int(&packet, &window_id, sizeof(window_id));
	packet_pack_int(&packet, &type, sizeof(type));
	packet_pack_string(&packet, title);
	packet_pack_int(&packet, &slots, sizeof(slots));
	packet_pack_int(&packet, &use_title, sizeof(use_title));

	client_send_packet(client, &packet);

	client->window_data.id = window_id;
	client->window_data.type = type;
}

