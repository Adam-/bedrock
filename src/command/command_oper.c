#include "server/command.h"

void command_oper(struct bedrock_client *client, int argc, const char **argv)
{
	const char *name = argv[1];
	const char *pass = argv[2];
	struct bedrock_oper *oper;
	bedrock_node *node;

	oper = oper_find(name);
	if (oper == NULL || strcmp(oper->password, pass))
	{
		command_reply(client, "Invalid username or password.");
		return;
	}

	client->oper = oper;

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;

		packet_send_chat_message(c, "%s is now an operator", client->name);
	}
}
