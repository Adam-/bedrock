#include "server/client.h"
#include "server/packet.h"
#include "entities/entity.h"

int packet_close_window(struct client *client, bedrock_packet *p)
{
	uint8_t window;

	packet_read_int(p, &window, sizeof(window));

	if (p->error || client->window_data.id != window)
		return ERROR_UNEXPECTED;

	if (client->window_data.entity != NULL)
	{
		bedrock_list_del(&client->window_data.entity->clients, client);
		client->window_data.entity = NULL;
	}

	memset(&client->window_data, 0, sizeof(client->window_data));

	return ERROR_OK;
}
