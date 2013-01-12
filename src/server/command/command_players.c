#include "server/command.h"

void command_players(struct command_source *source, int bedrock_attribute_unused argc, const char bedrock_attribute_unused **argv)
{
	bedrock_node *node;
	int players = 0, clients = 0;

	LIST_FOREACH(&client_list, node)
	{
		struct client *c = node->data;

		if (c->authenticated & STATE_IN_GAME)
		{
			command_reply(source, "%s", c->name);
			++players;
		}

		++clients;
	}

	command_reply(source, "Total players: %d, total clients: %d", players, clients);
}
