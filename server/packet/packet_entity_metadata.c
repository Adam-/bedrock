#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
#include "packet/packet_entity_metadata.h"

void packet_send_entity_metadata(struct client *client, enum entity_metadata_index index, enum entity_metadata_type type, const void *data, size_t size)
{
	uint8_t header = index | type << 5, footer = 127;
	bedrock_node *node;
	bedrock_packet packet;

	if (client->column == NULL)
		return;

	LIST_FOREACH(&client->column->players, node)
	{
		struct client *c = node->data;

		if (client == c)
			continue;

		packet_init(&packet, SERVER_ENTITY_METADATA);

		packet_pack_int(&packet, &client->id, sizeof(client->id));
		packet_pack_int(&packet, &header, sizeof(header));
		packet_pack(&packet, data, size);
		packet_pack_int(&packet, &footer, sizeof(footer));

		client_send_packet(c, &packet);
	}
}

void packet_send_entity_metadata_slot(struct client *client, struct dropped_item *di)
{
	uint8_t header = ENTITY_METADATA_INDEX_SLOT | ENTITY_METADATA_TYPE_SLOT << 5, footer = 127;
	bedrock_node *node;
	bedrock_packet packet;
	struct item_stack stack;

	if (client->column == NULL)
		return;

	stack.id = di->item->id;
	stack.count = di->count;
	stack.metadata = di->data;

	LIST_FOREACH(&client->column->players, node)
	{
		struct client *c = node->data;

		packet_init(&packet, SERVER_ENTITY_METADATA);

		packet_pack_int(&packet, &di->eid, sizeof(di->eid));
		packet_pack_int(&packet, &header, sizeof(header));
		packet_pack_slot(&packet, &stack);
		packet_pack_int(&packet, &footer, sizeof(footer));

		client_send_packet(c, &packet);
	}
}

