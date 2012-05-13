#include "packet/packet.h"

int packet_keep_alive(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	uint32_t id;
	size_t offset = 1;

	packet_read_int(buffer, len, &offset, &id, sizeof(id));

	client_send(client, &id, sizeof(id));

	bedrock_assert(offset == 5);
	return offset;
}

int packet_login_request(bedrock_client *client, const unsigned char *buffer, size_t len)
{
}

int packet_handshake(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	char username[BEDROCK_USERNAME_MAX];
	char *p;

	if (client->authenticated != STATE_UNAUTHENTICATED)
		return ERROR_UNEXPECTED;

	packet_read_string(buffer, len, &offset, username, sizeof(username));

	p = strchr(username, ';');
	if (p == NULL)
		return ERROR_INVALID_FORMAT;

	*p = 0;

	if (strlen(username) == 0)
		return ERROR_INVALID_FORMAT;

	// Check already exists

	strncpy(client->name, username, sizeof(client->name));
	client->authenticated = STATE_HANDSHAKING;

	client_send_header(client, HANDSHAKE);
	client_send_string(client, "-");

	return offset;
}
