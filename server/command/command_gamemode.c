#include "server/bedrock.h"
#include "server/client.h"
#include "server/command.h"
#include "packet/packet_chat_message.h"
#include "packet/packet_change_game_state.h"
#include "nbt/nbt.h"

#include <errno.h>

void command_gamemode(struct command_source *source, int bedrock_attribute_unused argc, const char **argv)
{
	struct client *targ = client_find(argv[1]);
	char *errptr;
	unsigned opt;
	bedrock_node *node;

	if (targ == NULL)
	{
		command_reply(source, "No such user: %s", argv[1]);
		return;
	}

	errno = 0;
	errptr = NULL;
	opt = strtoul(argv[2], &errptr, 10);
	if (errno || *errptr)
	{
		command_reply(source, "Game mode must be a number");
		return;
	}
	else if (opt > 1)
	{
		command_reply(source, "Game mode must be 0 or 1");
		return;
	}

	targ->gamemode = opt;
	packet_send_change_game_state(targ, CHANGE_GAME_MODE, opt);

	LIST_FOREACH(&client_list, node)
	{
		struct client *c = node->data;

		if (c->state & STATE_IN_GAME)
		{
			packet_send_chat_message(c, "%s changed game mode of %s to %d", source->user ? source->user->name : "[Server]", targ->name, opt);
		}
	}
}
