#include "server/client.h"
#include "server/packet.h"
#include "config/config.h"
#include "packet/packet_disconnect.h"
#include "packet/packet_encryption_request.h"

int packet_handshake(struct client *client, const bedrock_packet *p)
{
	int offset = PACKET_HEADER_LENGTH;
	struct world *world;
	uint8_t protocol;
	char username[BEDROCK_USERNAME_MAX];
	char server_host[64];
	uint32_t server_port;

	packet_read_int(p, &offset, &protocol, sizeof(protocol));
	packet_read_string(p, &offset, username, sizeof(username));
	packet_read_string(p, &offset, server_host, sizeof(server_host));
	packet_read_int(p, &offset, &server_port, sizeof(server_port));

	if (offset <= ERROR_UNKNOWN)
		return offset;

	if (client_valid_username(username) == false)
		return ERROR_INVALID_FORMAT;
	else if (protocol != BEDROCK_PROTOCOL_VERSION)
	{
		packet_send_disconnect(client, "Incorrect version");
		return offset;
	}
	else if (authenticated_client_count >= server_maxusers)
	{
		packet_send_disconnect(client, "Server is full");
		return offset;
	}
	else if (client_find(username) != NULL)
	{
		packet_send_disconnect(client, "Your account is already logged in");
		return offset;
	}
	else if (world_list.count == 0)
	{
		packet_send_disconnect(client, "Server misconfiguration - no default world");
		return offset;
	}

	world = world_list.head->data;
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

	packet_send_encryption_request(client);

	return offset;
}
