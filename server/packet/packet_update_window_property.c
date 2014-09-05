#include "server/client.h"
#include "server/packet.h"

void packet_send_update_window_property(struct client *client, int8_t window_id, int16_t property, int16_t value)
{
	bedrock_packet packet;

#if 0
	packet_init(&packet, SERVER_UPDATE_WINDOW_PROPERTY);

	packet_pack_byte(&packet, window_id);
	packet_pack_short(&packet, property);
	packet_pack_short(&packet, value);

	client_send_packet(client, &packet);
#endif
}
