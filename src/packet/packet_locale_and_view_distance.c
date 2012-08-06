#include "server/client.h"
#include "server/packet.h"

int packet_locale_and_view_distance(struct bedrock_client *client, const bedrock_packet *packet)
{
	size_t offset = PACKET_HEADER_LENGTH;
	char locale[BEDROCK_MAX_STRING_LENGTH];
	uint8_t view_distance, chat_flags, difficulty;

	packet_read_string(packet, &offset, locale, sizeof(locale));
	packet_read_int(packet, &offset, &view_distance, sizeof(view_distance));
	packet_read_int(packet, &offset, &chat_flags, sizeof(chat_flags));
	packet_read_int(packet, &offset, &difficulty, sizeof(difficulty));

	return offset;
}
