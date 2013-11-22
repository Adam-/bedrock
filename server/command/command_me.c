#include "server/bedrock.h"
#include "server/client.h"
#include "server/command.h"
#include "packet/packet_chat_message.h"

void command_me(struct command_source *source, int bedrock_attribute_unused argc, const char **argv)
{
	char final_message[BEDROCK_MAX_STRING_LENGTH];
	const char *message = argv[1];
	bedrock_node *node;

	{
		char *p = strrchr(message, SPECIAL_CHAR_1);
		if (p != NULL && (size_t) (p - message) == strlen(message) - 1)
			return;
	}

	if (source->user != NULL)
		snprintf(final_message, sizeof(final_message), "* %s %s", source->user->name, message);
	else if (source->console != NULL)
		snprintf(final_message, sizeof(final_message), "* [Server] %s", message);
	else
		return;

	LIST_FOREACH(&client_list, node)
	{
		struct client *c = node->data;

		if (c->state & STATE_IN_GAME)
		{
			packet_send_chat_message(c, "%s", final_message);
		}
	}
}
