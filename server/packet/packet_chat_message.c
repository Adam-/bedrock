#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "server/packets.h"
#include "server/command.h"

#include <jansson.h>

int packet_chat_message(struct client *client, bedrock_packet *p)
{
	char message[BEDROCK_MAX_STRING_LENGTH], final_message[BEDROCK_MAX_STRING_LENGTH];
	bedrock_node *node;

	packet_read_string(p, message, sizeof(message));

	if (p->error || !*message)
		return p->error;

	{
		char *ptr = strrchr(message, SPECIAL_CHAR_1);
		if (ptr != NULL && (size_t) (ptr - message) == strlen(message) - 1)
			return ERROR_INVALID_FORMAT;
	}

	if (*message == '/')
	{
		struct command_source source;
		
		source.user = client;
		source.console = NULL;

		bedrock_log(LEVEL_INFO, "command: %s: %s", client->name, message + 1);
		command_run(&source, message + 1);
		return p->error;
	}

	bedrock_log(LEVEL_INFO, "%s: %s", client->name, message);

	snprintf(final_message, sizeof(final_message), "<%s> %s", client->name, message);

	LIST_FOREACH(&client_list, node)
	{
		struct client *c = node->data;

		if (c->state & STATE_IN_GAME)
		{
			packet_send_chat_message(c, "%s", final_message);
		}
	}

	return p->error;
}

void packet_send_chat_message(struct client *client, const char *buf, ...)
{
	bedrock_packet packet;
	va_list args;
	char message[BEDROCK_MAX_STRING_LENGTH];
	json_t *j;
	char *c;

	va_start(args, buf);
	vsnprintf(message, sizeof(message), buf, args);
	va_end(args);

	j = json_pack("{"
				"s: s"
			"}",
				"text", message);
	c = json_dumps(j, 0);
	bedrock_assert(c != NULL, return;);

	packet_init(&packet, SERVER_CHAT_MESSAGE);
	packet_pack_string(&packet, c);
	client_send_packet(client, &packet);

	free(c);
	json_decref(j);
}
