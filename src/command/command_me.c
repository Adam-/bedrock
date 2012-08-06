#include "server/bedrock.h"
#include "server/client.h"
#include "server/command.h"
#include "packet/packet_chat_message.h"

void command_me(struct bedrock_client *client, int bedrock_attribute_unused argc, const char **argv)
{
	char final_message[BEDROCK_MAX_STRING_LENGTH];
	const char *message = argv[1];
	bedrock_node *node;

	{
		char *p = strrchr(message, SPECIAL_CHAR);
		if (p != NULL && (size_t) (p - message) == strlen(message) - 1)
			return;
	}

	snprintf(final_message, sizeof(final_message), "* %s %s", client->name, message);

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;

		if (c->authenticated & STATE_AUTHENTICATED)
		{
			packet_send_chat_message(c, "%s", final_message);
		}
	}
}
