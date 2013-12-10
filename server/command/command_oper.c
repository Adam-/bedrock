#include "server/command.h"
#include "server/packets.h"

void command_oper(struct command_source *source, int bedrock_attribute_unused argc, const char **argv)
{
	const char *name = argv[1];
	const char *pass = argv[2];
	struct oper *oper;
	bedrock_node *node;

	if (source->user == NULL)
	{
		command_reply(source, "This command is user-only!");
		return;
	}

	oper = oper_find(name);
	if (oper == NULL || strcmp(oper->password, pass))
	{
		command_reply(source, "Invalid username or password.");
		return;
	}

	source->user->oper = oper;

	LIST_FOREACH(&client_list, node)
	{
		struct client *c = node->data;

		packet_send_chat_message(c, "%s is now an operator", source->user->name);
	}
}
