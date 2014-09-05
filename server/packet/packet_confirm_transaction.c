#include "server/client.h"
#include "server/packet.h"

int packet_confirm_transaction(struct client bedrock_attribute_unused *client, bedrock_packet *p)
{
	int8_t b;
	int16_t s;

	packet_read_byte(p, &b);
	packet_read_short(p, &s);
	packet_read_byte(p, &b);

	return ERROR_OK;
}

void packet_send_confirm_transaction(struct client *client, uint8_t window, int16_t action_number, uint8_t accepted)
{
	bedrock_packet packet;

	packet_init(&packet, SERVER_CONFIRM_TRANSACTION);

	packet_pack_byte(&packet, window);
	packet_pack_short(&packet, action_number);
	packet_pack_bool(&packet, accepted);

	client_send_packet(client, &packet);
}
