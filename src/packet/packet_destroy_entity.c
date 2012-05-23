#include "server/client.h"
#include "packet/packet.h"

void packet_send_destroy_entity_player(struct bedrock_client *client, struct bedrock_client *c)
{
	client_send_header(client, DESTROY_ENTITY);
	client_send_int(client, &c->id, sizeof(c->id));
}

