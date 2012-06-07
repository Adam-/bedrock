#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "server/command.h"
#include "packet/packet_chat_message.h"

int packet_chat_message(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = PACKET_HEADER_LENGTH;
	char message[BEDROCK_MAX_STRING_LENGTH], final_message[1 + BEDROCK_USERNAME_MAX + 2 + BEDROCK_MAX_STRING_LENGTH];
	bedrock_node *node;

	packet_read_string(buffer, len, &offset, message, sizeof(message));

	if (offset <= ERROR_UNKNOWN || !*message)
		return offset;

	{
		char *p = strrchr(message, SPECIAL_CHAR);
		if (p != NULL && (size_t) (p - message) == strlen(message) - 1)
			return ERROR_INVALID_FORMAT;
	}

	if (*message == '/')
	{
		command_run(client, message + 1);
		return offset;
	}

	snprintf(final_message, sizeof(final_message), "<%s> %s", client->name, message);

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;

		if (c->authenticated == STATE_AUTHENTICATED)
		{
			packet_send_chat_message(c, "%s", final_message);
		}
	}

	return offset;
}

void packet_send_chat_message(struct bedrock_client *client, const char *buf, ...)
{
	va_list args;
	char message[BEDROCK_MAX_STRING_LENGTH];

	va_start(args, buf);
	vsnprintf(message, sizeof(message), buf, args);
	va_end(args);

	client_send_header(client, CHAT_MESSAGE);
	client_send_string(client, message);
}
