#include "server/client.h"
#include "server/packet.h"
#include "config/config.h"
#include "packet/packet_disconnect.h"
#include "packet/packet_encryption_request.h"
#include "packet/packet_join_game.h"
#include "packet/packet_login.h"
#include "util/string.h"

int packet_login(struct client *client, bedrock_packet *p)
{
	char username[BEDROCK_USERNAME_MAX];
	struct world *world;

	packet_read_string(p, username, sizeof(username));

	if (p->error)
		return p->error;

	if (client_valid_username(username) == false)
		return ERROR_INVALID_FORMAT;

	if (authenticated_client_count >= server_maxusers)
	{
		packet_send_disconnect(client, "Server is full");
		return ERROR_OK;
	}

	if (client_find(username) != NULL)
	{
		packet_send_disconnect(client, "Your account is already logged in");
		return ERROR_OK;
	}

	if (world_list.count == 0)
	{
		packet_send_disconnect(client, "Server misconfiguration - no default world");
		return ERROR_OK;
	}

	world = world_list.head->data;
	bedrock_assert(world != NULL, return ERROR_UNKNOWN);

	bedrock_strncpy(client->name, username, sizeof(client->name));
	client->state = STATE_LOGIN_HANDSHAKING;
	client->world = world;

	client_load(client);
	if (client->data == NULL)
	{
		if (allow_new_users == false)
		{
			packet_send_disconnect(client, "Unknown user");
			return ERROR_OK;
		}

		client_new(client);

		if (client->data == NULL)
		{
			packet_send_disconnect(client, "Error creating user");
			return ERROR_OK;
		}
	}

	//packet_send_encryption_request(client);
	packet_send_login_success(client);

	return ERROR_OK;
}

void packet_send_login_success(struct client *client)
{
	bedrock_packet packet;

	packet_init(&packet, LOGIN_SERVER_LOGIN_SUCCESS);
	packet_pack_string(&packet, client->name);
	packet_pack_string(&packet, client->name); // UID?
	client_send_packet(client, &packet);

	client->state = STATE_LOGGED_IN;
	packet_send_join_game(client);
}

