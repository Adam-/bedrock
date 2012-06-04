#include "server/client.h"
#include "packet/packet.h"
#include "nbt/nbt.h"
#include "packet/packet_disconnect.h"

int packet_login_request(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = PACKET_HEADER_LENGTH;
	int32_t version, i;
	char username[BEDROCK_USERNAME_MAX], unused[1];
	int8_t b;

	packet_read_int(buffer, len, &offset, &version, sizeof(version));
	packet_read_string(buffer, len, &offset, username, sizeof(username)); /* Username is sent here too, why? */
	packet_read_string(buffer, len, &offset, unused, sizeof(unused));
	packet_read_int(buffer, len, &offset, &i, sizeof(i));
	packet_read_int(buffer, len, &offset, &i, sizeof(i));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));

	if (offset <= ERROR_UNKNOWN)
		return offset;

	if (version != BEDROCK_PROTOCOL_VERSION)
	{
		packet_send_disconnect(client, "Incorrect version");
		return offset;
	}
	else if (strcmp(client->name, username))
	{
		packet_send_disconnect(client, "Username mismatch");
		return offset;
	}

	client_send_header(client, LOGIN_REQUEST);
	client_send_int(client, &client->id, sizeof(client->id)); /* Entity ID */
	client_send_string(client, "");
	client_send_string(client, nbt_read_string(client->world->data, 2, "Data", "generatorName")); /* Generator name */
	client_send_int(client, nbt_read(client->world->data, TAG_INT, 2, "Data", "GameType"), sizeof(uint32_t)); /* Game type */
	client_send_int(client, nbt_read(client->data, TAG_INT, 1, "Dimension"), sizeof(uint32_t)); /* Dimension */
	client_send_int(client, nbt_read(client->world->data, TAG_BYTE, 2, "Data", "hardcore"), sizeof(uint8_t)); /* hardcore */
	b = 0;
	client_send_int(client, &b, sizeof(b)); /* Not used */
	b = BEDROCK_MAX_USERS;
	client_send_int(client, &b, sizeof(b)); /* Max players */

	client->authenticated = STATE_BURSTING;
	client_start_login_sequence(client);

	return offset;
}
