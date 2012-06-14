#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_disconnect.h"

int packet_handshake(struct bedrock_client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	struct bedrock_world *world;
	char username[BEDROCK_USERNAME_MAX + 1 + 64 + 1 + 5];
	char *s;
	bedrock_packet packet;

	packet_read_string(p, &offset, username, sizeof(username));

	if (offset <= ERROR_UNKNOWN)
		return offset;

	s = strchr(username, ';');
	if (s == NULL)
		return ERROR_INVALID_FORMAT;

	*s = 0;

	if (client_valid_username(username) == false)
		return ERROR_INVALID_FORMAT;
	else if (client_find(username) != NULL)
	{
		packet_send_disconnect(client, "Your account is already logged in");
		return offset;
	}

	world = world_find(BEDROCK_WORLD_NAME);
	bedrock_assert(world != NULL, return ERROR_UNKNOWN);

	strncpy(client->name, username, sizeof(client->name));
	client->authenticated = STATE_HANDSHAKING;
	client->world = world;

	client_load(client);
	if (client->data == NULL)
	{
		packet_send_disconnect(client, "Unknown user");
		return offset;
	}

	packet_init(&packet, HANDSHAKE);

	packet_pack_header(&packet, HANDSHAKE);
	packet_pack_string(&packet, "-");

	client_send_packet(client, &packet);

	return offset;
}
