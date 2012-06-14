#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"

void packet_send_destroy_entity_player(struct bedrock_client *client, struct bedrock_client *c)
{
	bedrock_packet packet;

	packet_init(&packet, DESTROY_ENTITY);

	packet_pack_header(&packet, DESTROY_ENTITY);
	packet_pack_int(&packet, &c->id, sizeof(c->id));

	client_send_packet(client, &packet);
}

void packet_send_destroy_entity_dropped_item(struct bedrock_client *client, struct bedrock_dropped_item *di)
{
	bedrock_packet packet;

	packet_init(&packet, DESTROY_ENTITY);

	packet_pack_header(&packet, DESTROY_ENTITY);
	packet_pack_int(&packet, &di->eid, sizeof(di->eid));

	client_send_packet(client, &packet);
}
