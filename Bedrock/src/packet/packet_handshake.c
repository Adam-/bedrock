#include "server/client.h"
#include "packet/packet.h"

int packet_handshake(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	struct bedrock_world *world;
	char username[BEDROCK_USERNAME_MAX + 1 + 64 + 1 + 5];
	char *p;

	packet_read_string(buffer, len, &offset, username, sizeof(username));

	if (offset == ERROR_EAGAIN)
		return ERROR_EAGAIN;

	p = strchr(username, ';');
	if (p == NULL)
		return ERROR_INVALID_FORMAT;

	*p = 0;

	if (client_valid_username(username) == false)
		return ERROR_INVALID_FORMAT;

	// Check already exists

	world = world_find(BEDROCK_WORLD_NAME);
	bedrock_assert_ret(world != NULL, ERROR_UNKNOWN);

	strncpy(client->name, username, sizeof(client->name));
	client->authenticated = STATE_HANDSHAKING;
	client->world = world;

	client_load(client); // Can fail if a new client
	assert(client->data); // XXX

	client_send_header(client, HANDSHAKE);
	client_send_string(client, "-");

	return offset;
}
