#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "nbt/nbt.h"
#include "config/config.h"

void packet_send_login_request(struct bedrock_client *client)
{
	bedrock_packet packet;
	uint8_t b;

	packet_init(&packet, LOGIN_REQUEST);

	packet_pack_header(&packet, LOGIN_REQUEST);
	packet_pack_int(&packet, &client->id, sizeof(client->id)); /* Entity ID */
	packet_pack_string(&packet, nbt_read_string(client->world->data, 2, "Data", "generatorName")); /* Generator name */
	packet_pack_int(&packet, nbt_read(client->world->data, TAG_INT, 2, "Data", "GameType"), sizeof(uint32_t)); /* Game type */
	packet_pack_int(&packet, nbt_read(client->data, TAG_INT, 1, "Dimension"), sizeof(uint32_t)); /* Dimension */
	packet_pack_int(&packet, nbt_read(client->world->data, TAG_BYTE, 2, "Data", "hardcore"), sizeof(uint8_t)); /* hardcore */
	b = 0;
	packet_pack_int(&packet, &b, sizeof(b)); /* Not used */
	b = server_maxusers;
	packet_pack_int(&packet, &b, sizeof(b)); /* Max players */

	client_send_packet(client, &packet);

	bedrock_assert(client->authenticated == STATE_LOGGED_IN, ;);
	client->authenticated = STATE_BURSTING;
	++authenticated_client_count;
	client_start_login_sequence(client);

	bedrock_log(LEVEL_INFO, "client: %s logged in from %s", client->name, client_get_ip(client));
}
