#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_entity_metadata.h"

void packet_send_entity_metadata(struct bedrock_client *client, entity_metadata_index index, entity_metadata_type type, const void *data, size_t size)
{
	uint8_t header = index | type << 5, footer = 127;
	bedrock_node *node;

	LIST_FOREACH(&client->players, node)
	{
		struct bedrock_client *c = node->data;

		client_send_header(c, ENTITY_METADATA);
		client_send_int(c, &client->id, sizeof(client->id));
		client_send_int(c, &header, sizeof(header));
		client_send(c, data, size);
		client_send_int(c, &footer, sizeof(footer));
	}
}
