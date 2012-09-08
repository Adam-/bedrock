#include "server/client.h"
#include "server/packet.h"
#include "config/config.h"
#include "server/io.h"
#include "packet/packet_chat_message.h"

int packet_list_ping(struct bedrock_client *client, const unsigned char bedrock_attribute_unused *buffer, size_t __attribute__((__unused__)) len)
{
	size_t offset = PACKET_HEADER_LENGTH;
	char string[BEDROCK_MAX_STRING_LENGTH];
	bedrock_packet packet;

	snprintf(string, sizeof(string), "%s%c%d%c%d", server_desc, SPECIAL_CHAR, authenticated_client_count, SPECIAL_CHAR, server_maxusers);

	packet_init(&packet, DISCONNECT);

	packet_pack_header(&packet, DISCONNECT);
	packet_pack_string(&packet, string);

	client_send_packet(client, &packet);

	io_disable(&client->fd.event_read);

	return offset;
}
