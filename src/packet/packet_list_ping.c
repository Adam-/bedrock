#include "server/client.h"
#include "server/packet.h"
#include "packet/packet_disconnect.h"
#include "packet/packet_chat_message.h"

int packet_list_ping(struct bedrock_client *client, const unsigned char __attribute__((__unused__)) *buffer, size_t __attribute__((__unused__)) len)
{
	size_t offset = PACKET_HEADER_LENGTH;
	char string[BEDROCK_MAX_STRING_LENGTH];

	snprintf(string, sizeof(string), "%s%c%d%c%d", BEDROCK_DESCRIPTION, SPECIAL_CHAR, authenticated_client_count, SPECIAL_CHAR, BEDROCK_MAX_USERS);
	packet_send_disconnect(client, string);

	return offset;
}
