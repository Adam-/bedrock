#include "server/client.h"
#include "packet/packet.h"

void packet_send_player_list_item(struct bedrock_client *client, const char *player, uint8_t online, uint16_t ping)
{
	client_send_header(client, PLAYER_LIST);
	client_send_string(client, player);
	client_send_int(client, &online, sizeof(online));
	client_send_int(client, &ping, sizeof(ping));
}
