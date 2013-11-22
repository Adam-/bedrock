#include "server/client.h"
#include "server/packet.h"

void packet_send_update_window_property(struct client *client, int8_t window_id, int16_t property, int16_t value)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_UPDATE_WINDOW_PROPERTY);

	packet_pack_int(&packet, &window_id, sizeof(window_id));
	packet_pack_int(&packet, &property, sizeof(property));
	packet_pack_int(&packet, &value, sizeof(value));

	client_send_packet(client, &packet);
}
