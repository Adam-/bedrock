#include "server/client.h"
#include "packet/packet.h"

int packet_held_item_change(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = PACKET_HEADER_LENGTH;

	packet_read_int(buffer, len, &offset, &client->selected_slot, sizeof(client->selected_slot));

	return offset;
}
