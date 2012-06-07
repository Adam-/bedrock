#include "server/client.h"
#include "server/packet.h"

void packet_send_player_list_item(struct bedrock_client *client, struct bedrock_client *c, uint8_t online)
{
	client_send_header(client, PLAYER_LIST);
	client_send_string(client, c->name);
	client_send_int(client, &online, sizeof(online));
	client_send_int(client, &c->ping, sizeof(c->ping));
}
