#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"

void packet_send_destroy_entity_player(struct client *client, struct client *c)
{
	bedrock_packet packet;
	uint8_t count = 1;

	packet_init(&packet, SERVER_DESTROY_ENTITY);

	packet_pack_int(&packet, &count, sizeof(count));
	packet_pack_int(&packet, &c->id, sizeof(c->id));

	client_send_packet(client, &packet);
}

void packet_send_destroy_entity_dropped_item(struct client *client, struct dropped_item *di)
{
	bedrock_packet packet;
	uint8_t count = 1;

	packet_init(&packet, SERVER_DESTROY_ENTITY);

	packet_pack_int(&packet, &count, sizeof(count));
	packet_pack_int(&packet, &di->p.id, sizeof(di->p.id));

	client_send_packet(client, &packet);
}

void packet_send_destroy_entity_projectile(struct client *client, struct projectile *p)
{
	bedrock_packet packet;
	uint8_t count = 1;

	packet_init(&packet, SERVER_DESTROY_ENTITY);

	packet_pack_int(&packet, &count, sizeof(count));
	packet_pack_int(&packet, &p->id, sizeof(p->id));

	client_send_packet(client, &packet);
}

