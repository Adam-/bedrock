#include "packet/packet.h"

int packet_keep_alive(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	uint32_t id;

	packet_read_int(buffer, len, &offset, &id, sizeof(id));

	client_send_int(client, &id, sizeof(id));

	bedrock_assert(offset == 5);
	return offset;
}

int packet_login_request(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	int32_t version, i;
	char string[BEDROCK_USERNAME_MAX];
	int8_t b;
	static uint32_t entity_id = 0;

	if (client->authenticated != STATE_HANDSHAKING)
		return ERROR_UNEXPECTED;

	packet_read_int(buffer, len, &offset, &version, sizeof(version));
	packet_read_string(buffer, len, &offset, string, sizeof(string)); /* Username is sent here too, why? */
	packet_read_string(buffer, len, &offset, string, sizeof(string));
	packet_read_int(buffer, len, &offset, &i, sizeof(i));
	packet_read_int(buffer, len, &offset, &i, sizeof(i));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));

	// Check version, should be 29

	client->authenticated = STATE_AUTHENTICATED;

	client_send_header(client, LOGIN_REQUEST);
	client_send_int(client, ++entity_id, sizeof(entity_id));
	client_send_string(client, "");

	return offset;
}

int packet_handshake(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	bedrock_world *world;
	char username[BEDROCK_USERNAME_MAX];
	char *p;

	if (client->authenticated != STATE_UNAUTHENTICATED)
		return ERROR_UNEXPECTED;

	packet_read_string(buffer, len, &offset, username, sizeof(username));

	p = strchr(username, ';');
	if (p == NULL)
		return ERROR_INVALID_FORMAT;

	*p = 0;

	if (client_valid_username(username) == false)
		return ERROR_INVALID_FORMAT;

	// Check already exists

	world = world_find(BEDROCK_WORLD_NAME);
	bedrock_assert_ret(world != NULL, -1);

	strncpy(client->name, username, sizeof(client->name));
	client->authenticated = STATE_HANDSHAKING;
	client->world = world;

	client_send_header(client, HANDSHAKE);
	client_send_string(client, "-");

	return offset;
}
