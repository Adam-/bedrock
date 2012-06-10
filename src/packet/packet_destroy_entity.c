#include "server/client.h"
#include "server/packet.h"
#include "blocks/items.h"

void packet_send_destroy_entity_player(struct bedrock_client *client, struct bedrock_client *c)
{
	client_send_header(client, DESTROY_ENTITY);
	client_send_int(client, &c->id, sizeof(c->id));
}

void packet_send_destroy_entity_dropped_item(struct bedrock_client *client, struct bedrock_dropped_item *di)
{
	client_send_header(client, DESTROY_ENTITY);
	client_send_int(client, &di->eid, sizeof(di->eid));
}
