#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "server/command.h"
#include "packet/packet_chat_message.h"

int packet_chat_message(struct client *client, const bedrock_packet *p)
{
	int offset = PACKET_HEADER_LENGTH;
	char message[BEDROCK_MAX_STRING_LENGTH], final_message[BEDROCK_MAX_STRING_LENGTH];
	bedrock_node *node;

	packet_read_string(p, &offset, message, sizeof(message));

	if (offset <= ERROR_UNKNOWN || !*message)
		return offset;

	{
		char *p = strrchr(message, SPECIAL_CHAR);
		if (p != NULL && (size_t) (p - message) == strlen(message) - 1)
			return ERROR_INVALID_FORMAT;

		p = strchr(message, '"'); // XXX
		if (p != NULL)
			return offset;
	}

	if (*message == '/')
	{
		struct command_source source;
		
		source.user = client;
		source.console = NULL;

		bedrock_log(LEVEL_INFO, "command: %s: %s", client->name, message + 1);
		command_run(&source, message + 1);
		return offset;
	}

	bedrock_log(LEVEL_INFO, "%s: %s", client->name, message);

	snprintf(final_message, sizeof(final_message), "<%s> %s", client->name, message);

	LIST_FOREACH(&client_list, node)
	{
		struct client *c = node->data;

		if (c->authenticated & STATE_IN_GAME)
		{
			packet_send_chat_message(c, "%s", final_message);
		}
	}

	return offset;
}

void packet_send_chat_message(struct client *client, const char *buf, ...)
{
	bedrock_packet packet;
	va_list args;
	char message[BEDROCK_MAX_STRING_LENGTH], json_message[BEDROCK_MAX_STRING_LENGTH];

	va_start(args, buf);
	vsnprintf(message, sizeof(message), buf, args);
	va_end(args);

	snprintf(json_message, sizeof(json_message), "{\"text\":\"%s\"}", message);

	packet_init(&packet, CHAT_MESSAGE);

	packet_pack_header(&packet, CHAT_MESSAGE);
	packet_pack_string(&packet, json_message);

	client_send_packet(client, &packet);
}
