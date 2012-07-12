#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
#include "packet/packet_entity_metadata.h"

void packet_send_entity_metadata(struct bedrock_client *client, entity_metadata_index index, entity_metadata_type type, const void *data, size_t size)
{
	uint8_t header = index | type << 5, footer = 127;
	bedrock_node *node;
	bedrock_packet packet;

	LIST_FOREACH(&client->column->players, node)
	{
		struct bedrock_client *c = node->data;

		packet_init(&packet, ENTITY_METADATA);

		packet_pack_header(&packet, ENTITY_METADATA);
		packet_pack_int(&packet, &client->id, sizeof(client->id));
		packet_pack_int(&packet, &header, sizeof(header));
		packet_pack(&packet, data, size);
		packet_pack_int(&packet, &footer, sizeof(footer));

		client_send_packet(c, &packet);
	}
}
