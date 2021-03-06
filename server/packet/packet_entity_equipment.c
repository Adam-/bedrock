#include "server/client.h"
#include "server/packet.h"
#include "blocks/items.h"

void packet_send_entity_equipment(struct client *client, struct client *c, uint16_t slot, struct item *item, uint16_t damage)
{
	bedrock_packet packet;
	struct item_stack stack;

	stack.id = item->id ? item->id : -1;
	stack.count = 1;
	stack.metadata = damage;

	packet_init(&packet, SERVER_ENTITY_EQUIPMENT);

	packet_pack_varint(&packet, c->id);
	packet_pack_short(&packet, slot);
	packet_pack_slot(&packet, &stack);

	client_send_packet(client, &packet);
}
