#include "server/client.h"
#include "server/packet.h"
#include "nbt/nbt.h"
#include "packet/packet_disconnect.h"

int packet_login_request(struct bedrock_client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	int32_t version, i;
	char username[BEDROCK_USERNAME_MAX], unused[1];
	int8_t b;
	bedrock_packet packet;

	packet_read_int(p, &offset, &version, sizeof(version));
	packet_read_string(p, &offset, username, sizeof(username)); /* Username is sent here too, why? */
	packet_read_string(p, &offset, unused, sizeof(unused));
	packet_read_int(p, &offset, &i, sizeof(i));
	packet_read_int(p, &offset, &i, sizeof(i));
	packet_read_int(p, &offset, &b, sizeof(b));
	packet_read_int(p, &offset, &b, sizeof(b));
	packet_read_int(p, &offset, &b, sizeof(b));

	if (offset <= ERROR_UNKNOWN)
		return offset;

	if (version < BEDROCK_PROTOCOL_VERSION)
	{
		packet_send_disconnect(client, "Incorrect version");
		return offset;
	}
	else if (strcmp(client->name, username))
	{
		packet_send_disconnect(client, "Username mismatch");
		return offset;
	}
	else if (authenticated_client_count >= BEDROCK_MAX_USERS)
	{
		packet_send_disconnect(client, "Server is full");
		return offset;
	}

	packet_init(&packet, LOGIN_REQUEST);

	packet_pack_header(&packet, LOGIN_REQUEST);
	packet_pack_int(&packet, &client->id, sizeof(client->id)); /* Entity ID */
	packet_pack_string(&packet, "");
	packet_pack_string(&packet, nbt_read_string(client->world->data, 2, "Data", "generatorName")); /* Generator name */
	packet_pack_int(&packet, nbt_read(client->world->data, TAG_INT, 2, "Data", "GameType"), sizeof(uint32_t)); /* Game type */
	packet_pack_int(&packet, nbt_read(client->data, TAG_INT, 1, "Dimension"), sizeof(uint32_t)); /* Dimension */
	packet_pack_int(&packet, nbt_read(client->world->data, TAG_BYTE, 2, "Data", "hardcore"), sizeof(uint8_t)); /* hardcore */
	b = 0;
	packet_pack_int(&packet, &b, sizeof(b)); /* Not used */
	b = BEDROCK_MAX_USERS;
	packet_pack_int(&packet, &b, sizeof(b)); /* Max players */

	client_send_packet(client, &packet);

	client->authenticated = STATE_BURSTING;
	++authenticated_client_count;
	client_start_login_sequence(client);

	return offset;
}
