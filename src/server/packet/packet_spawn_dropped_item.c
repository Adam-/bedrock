#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"

void packet_send_spawn_dropped_item(struct client *client, struct dropped_item *di)
{
	bedrock_packet packet;
	struct item_stack stack;
	int32_t a_x, a_y, a_z;
	uint8_t b;

	packet_init(&packet, SPAWN_DROPPED_ITEM);

	packet_pack_header(&packet, SPAWN_DROPPED_ITEM);
	packet_pack_int(&packet, &di->eid, sizeof(di->eid));

	stack.id = di->item->id;
	stack.count = di->count;
	stack.metadata = di->data;
	packet_pack_slot(&packet, &stack);

	a_x = (int) di->x * 32;
	a_y = (int) di->y * 32;
	a_z = (int) di->z * 32;

	packet_pack_int(&packet, &a_x, sizeof(a_x));
	packet_pack_int(&packet, &a_y, sizeof(a_y));
	packet_pack_int(&packet, &a_z, sizeof(a_z));

	b = 0;
	packet_pack_int(&packet, &b, sizeof(b));
	packet_pack_int(&packet, &b, sizeof(b));
	packet_pack_int(&packet, &b, sizeof(b));

	client_send_packet(client, &packet);
}
