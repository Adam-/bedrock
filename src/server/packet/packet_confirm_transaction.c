#include "server/client.h"
#include "server/packet.h"

void packet_send_confirm_transaction(struct client *client, uint8_t window, int16_t action_number, uint8_t accepted)
{
	bedrock_packet packet;

	packet_init(&packet, CONFIRM_TRANSACTION);

	packet_pack_header(&packet, CONFIRM_TRANSACTION);
	packet_pack_int(&packet, &window, sizeof(window));
	packet_pack_int(&packet, &action_number, sizeof(action_number));
	packet_pack_int(&packet, &accepted, sizeof(accepted));

	client_send_packet(client, &packet);
}
