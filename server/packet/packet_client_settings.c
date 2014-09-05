#include "server/client.h"
#include "server/packet.h"

int packet_client_settings(struct client *client, bedrock_packet *packet)
{
	char locale[BEDROCK_MAX_STRING_LENGTH];
	bool chat_colors;
	int8_t view_distance, chat_flags, displayed_skin_parts;

	packet_read_string(packet, locale, sizeof(locale));
	packet_read_byte(packet, &view_distance);
	packet_read_byte(packet, &chat_flags);
	packet_read_bool(packet, &chat_colors);
	packet_read_byte(packet, &displayed_skin_parts);

	if (packet->error == ERROR_OK)
	{
		//if (view_distance > BEDROCK_MAX_VIEW_LENGTH)
		//	return ERROR_NOT_ALLOWED;

		client->view_distance = 8;//view_distance;
	}

	return ERROR_OK;
}
