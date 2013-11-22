#include "server/client.h"
#include "server/packet.h"

#define min(x, y) ((x) < (y) ? (x) : (y))

int packet_plugin_message(struct client bedrock_attribute_unused *client, bedrock_packet *p)
{
	char string[BEDROCK_MAX_STRING_LENGTH];
	int16_t s;

	packet_read_string(p, string, sizeof(string));
	packet_read_int(p, &s, sizeof(s));
	while (s > 0)
	{
		int len = min(s, (int) sizeof(string));
		packet_read(p, string, len);
		s -= len;
	}

	return ERROR_OK;
}
