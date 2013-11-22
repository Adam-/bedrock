#include "server/client.h"
#include "server/packet.h"

int packet_client_settings(struct client bedrock_attribute_unused *client, bedrock_packet *packet)
{
	char locale[BEDROCK_MAX_STRING_LENGTH];
	uint8_t view_distance, chat_flags, unused, difficulty, cape;

	packet_read_string(packet, locale, sizeof(locale));
	packet_read_int(packet, &view_distance, sizeof(view_distance));
	packet_read_int(packet, &chat_flags, sizeof(chat_flags));
	packet_read_int(packet, &unused, sizeof(unused));
	packet_read_int(packet, &difficulty, sizeof(difficulty));
	packet_read_int(packet, &cape, sizeof(cape));

	return ERROR_OK;
}
