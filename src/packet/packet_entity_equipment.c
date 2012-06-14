#include "server/client.h"
#include "server/packet.h"
#include "blocks/items.h"

void packet_send_entity_equipment(struct bedrock_client *client, struct bedrock_client *c, uint16_t slot, struct bedrock_item *item, uint16_t damage)
{
	bedrock_packet packet;

	packet_init(&packet, ENTITY_EQUIPMENT);

	packet_pack_header(&packet, ENTITY_EQUIPMENT);
	packet_pack_int(&packet, &c->id, sizeof(c->id));
	packet_pack_int(&packet, &slot, sizeof(slot));
	packet_pack_int(&packet, &item->id, sizeof(item->id));
	packet_pack_int(&packet, &damage, sizeof(damage));

	client_send_packet(client, &packet);
}
