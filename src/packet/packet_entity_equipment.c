#include "server/client.h"
#include "server/packet.h"
#include "blocks/items.h"

void packet_send_entity_equipment(struct bedrock_client *client, struct bedrock_client *c, uint16_t slot, struct bedrock_item *item, uint16_t damage)
{
	client_send_header(client, ENTITY_EQUIPMENT);
	client_send_int(client, &c->id, sizeof(c->id));
	client_send_int(client, &slot, sizeof(slot));
	client_send_int(client, &item->id, sizeof(item->id));
	client_send_int(client, &damage, sizeof(damage));
}
