#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"

void packet_send_collect_item(struct client *client, struct dropped_item *di)
{
	bedrock_node *node;

	LIST_FOREACH(&di->p.column->players, node)
	{
		struct client *c = node->data;

		bedrock_packet packet;

		packet_init(&packet, SERVER_COLLECT_ITEM);

		packet_pack_varint(&packet, di->p.id);
		packet_pack_varint(&packet, client->id);

		client_send_packet(c, &packet);
	}
}
