#include "server/client.h"
#include "packet/packet.h"
#include "server/bedrock.h"
#include "packet/packet_chat_message.h"

int packet_chat_message(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	char message[BEDROCK_MAX_STRING_LENGTH], final_message[1 + BEDROCK_USERNAME_MAX + 2 + BEDROCK_MAX_STRING_LENGTH];
	bedrock_node *node;

	packet_read_string(buffer, len, &offset, message, sizeof(message));

	if (!*message)
		return ERROR_INVALID_FORMAT;

	snprintf(final_message, sizeof(final_message), "<%s> %s", client->name, message);

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;

		if (c->authenticated == STATE_AUTHENTICATED)
		{
			packet_send_chat_message(c, final_message);
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
