#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"

void packet_send_spawn_dropped_item(struct bedrock_client *client, struct bedrock_dropped_item *di)
{
	int32_t a_x, a_y, a_z;
	uint8_t b;

	client_send_header(client, SPAWN_DROPPED_ITEM);
	client_send_int(client, &di->eid, sizeof(di->eid));
	client_send_int(client, &di->item->id, sizeof(di->item->id));
	client_send_int(client, &di->count, sizeof(di->count));
	client_send_int(client, &di->data, sizeof(di->data));

	a_x = (int) di->x * 32;
	a_y = (int) di->y * 32;
	a_z = (int) di->z * 32;

	client_send_int(client, &a_x, sizeof(a_x));
	client_send_int(client, &a_y, sizeof(a_y));
	client_send_int(client, &a_z, sizeof(a_z));

	b = 0;
	client_send_int(client, &b, sizeof(b));
	client_send_int(client, &b, sizeof(b));
	client_send_int(client, &b, sizeof(b));
}
