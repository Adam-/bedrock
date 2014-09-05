#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"

void packet_send_destroy_entity_player(struct client *client, struct client *c)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_DESTROY_ENTITY);

	packet_pack_byte(&packet, 1);
	packet_pack_varint(&packet, c->id);

	client_send_packet(client, &packet);
}

void packet_send_destroy_entity_dropped_item(struct client *client, struct dropped_item *di)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_DESTROY_ENTITY);

	packet_pack_byte(&packet, 1);
	packet_pack_varint(&packet, di->p.id);

	client_send_packet(client, &packet);
}

void packet_send_destroy_entity_projectile(struct client *client, struct projectile *p)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_DESTROY_ENTITY);

	packet_pack_byte(&packet, 1);
	packet_pack_varint(&packet, p->id);

	client_send_packet(client, &packet);
}

