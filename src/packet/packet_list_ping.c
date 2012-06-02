#include "server/client.h"
#include "packet/packet.h"
#include "packet/packet_disconnect.h"
#include "packet/packet_chat_message.h"

int packet_list_ping(struct bedrock_client *client, const unsigned char __attribute__((__unused__)) *buffer, size_t __attribute__((__unused__)) len)
{
	size_t offset = PACKET_HEADER_LENGTH;
	char string[BEDROCK_MAX_STRING_LENGTH];
	bedrock_node *node;
	uint32_t count = 0;

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;

		if (c->authenticated == STATE_AUTHENTICATED)
			++count;
	}

	snprintf(string, sizeof(string), "%s%c%d%c%d", BEDROCK_DESCRIPTION, SPECIAL_CHAR, count, SPECIAL_CHAR, BEDROCK_MAX_USERS);
	packet_send_disconnect(client, string);

	return offset;
}
