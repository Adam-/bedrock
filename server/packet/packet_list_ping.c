#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "config/config.h"
#include "packet/packet_chat_message.h"
#include "util/io.h"

int packet_list_ping(struct client *client, const bedrock_packet *p)
{
	int offset = PACKET_HEADER_LENGTH;
	uint8_t b;
	char string[BEDROCK_MAX_STRING_LENGTH];
	int len = 0;
	bedrock_packet packet;

	packet_read_int(p, &offset, &b, sizeof(b));

	string[len++] = (char) SPECIAL_CHAR;
	string[len++] = '1';
	string[len++] = 0;
	len += snprintf(string + len, sizeof(string) - len, "%d", BEDROCK_PROTOCOL_VERSION);
	string[len++] = 0;
	len += snprintf(string + len, sizeof(string) - len, "bedrock-%d.%d%s", BEDROCK_VERSION_MAJOR, BEDROCK_VERSION_MINOR, BEDROCK_VERSION_EXTRA);
	string[len++] = 0;
	strncpy(string + len, server_desc, sizeof(string) - len);
	len += strlen(server_desc) + 1;
	len += snprintf(string + len, sizeof(string) - len, "%d", authenticated_client_count);
	string[len++] = 0;
	len += snprintf(string + len, sizeof(string) - len, "%d", server_maxusers);

	packet_init(&packet, DISCONNECT);

	packet_pack_header(&packet, DISCONNECT);
	packet_pack_string_len(&packet, string, len);

	client_send_packet(client, &packet);

	io_disable(&client->fd.event_read);

	return offset;
}
