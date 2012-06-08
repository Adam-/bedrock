#include "server/client.h"
#include "server/packet.h"

void packet_send_block_change(struct bedrock_client *client, int32_t x, uint8_t y, int32_t z, uint8_t id, uint8_t data)
{
	client_send_header(client, BLOCK_CHANGE);
	client_send_int(client, &x, sizeof(x));
	client_send_int(client, &y, sizeof(y));
	client_send_int(client, &z, sizeof(z));
	client_send_int(client, &id, sizeof(id));
	client_send_int(client, &data, sizeof(data));
}
