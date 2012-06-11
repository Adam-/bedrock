#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"

void packet_send_collect_item(struct bedrock_client *client, struct bedrock_dropped_item *di)
{
	bedrock_node *node;

	LIST_FOREACH(&di->column->players, node)
	{
		struct bedrock_client *c = node->data;

		client_send_header(c, COLLECT_ITEM);
		client_send_int(c, &di->eid, sizeof(di->eid));
		client_send_int(c, &client->id, sizeof(client->id));
	}
}
