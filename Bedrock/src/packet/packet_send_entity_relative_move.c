#include "server/client.h"
#include "packet/packet.h"

void packet_send_entity_relative_move(struct bedrock_client *client, struct bedrock_client *targ, int8_t c_x, int8_t c_y, int8_t c_z, int8_t yaw, int8_t pitch)
{
	client_send_header(client, ENTITY_RELATIVE_MOVE);
	client_send_int(client, &targ->id, sizeof(targ->id));
	client_send_int(client, &c_x, sizeof(c_x));
	client_send_int(client, &c_y, sizeof(c_y));
	client_send_int(client, &c_z, sizeof(c_z));
}
