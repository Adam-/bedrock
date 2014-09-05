#include "server/client.h"
#include "server/packet.h"

void packet_send_player_list_item(struct client *client, struct client *c, uint8_t online)
{
	bedrock_packet packet;

#if 0
	packet_init(&packet, SERVER_PLAYER_LIST);

	packet_pack_string(&packet, c->name);
	packet_pack_byte(&packet, online);
	packet_pack_short(&packet, c->ping);

	client_send_packet(client, &packet);
#endif
}
