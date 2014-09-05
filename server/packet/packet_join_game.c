#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "nbt/nbt.h"
#include "config/config.h"

void packet_send_join_game(struct client *client)
{
	bedrock_packet packet;
	int32_t dimension;

	nbt_copy(client->data, TAG_INT, &dimension, sizeof(dimension), 1, "Dimension");

	packet_init(&packet, SERVER_JOIN_GAME);

	packet_pack_int(&packet, client->id); /* Entity ID */
	packet_pack_byte(&packet, client->gamemode);
	packet_pack_byte(&packet, dimension);
	packet_pack_integer(&packet, nbt_read(client->world->data, TAG_BYTE, 2, "Data", "hardcore"), sizeof(uint8_t)); /* hardcore */
	packet_pack_byte(&packet, server_maxusers); /* Max players */
	packet_pack_string(&packet, nbt_read_string(client->world->data, 2, "Data", "generatorName")); /* Level type */
	packet_pack_bool(&packet, false);

	client_send_packet(client, &packet);

	bedrock_assert(client->state == STATE_LOGGED_IN, ;);
	client->state = STATE_BURSTING;
	++authenticated_client_count;
	client_start_login_sequence(client);

	bedrock_log(LEVEL_INFO, "client: %s logged in from %s", client->name, client_get_ip(client));
}
