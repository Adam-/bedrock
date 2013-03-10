#include "server/client.h"
#include "server/packet.h"
#include "blocks/items.h"

void packet_send_entity_equipment(struct client *client, struct client *c, uint16_t slot, struct item *item, uint16_t damage)
{
	bedrock_packet packet;
	struct item_stack stack;

	stack.id = item->id;
	stack.count = 1;
	stack.metadata = damage;

	packet_init(&packet, ENTITY_EQUIPMENT);

	packet_pack_header(&packet, ENTITY_EQUIPMENT);
	packet_pack_int(&packet, &c->id, sizeof(c->id));
	packet_pack_int(&packet, &slot, sizeof(slot));
	packet_pack_slot(&packet, &stack);

	client_send_packet(client, &packet);
}
