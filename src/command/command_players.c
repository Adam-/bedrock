#include "server/command.h"

void command_players(struct bedrock_client *client, int argc, const char **argv)
{
	bedrock_node *node;
	int players = 0, clients = 0;

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;

		if (c->authenticated == STATE_AUTHENTICATED)
		{
			command_reply(client, "%s", c->name);
			++players;
		}

		++clients;
	}

	command_reply(client, "Total players: %d, total clients: %d", players, clients);
}
