#include "server/client.h"
#include "packet/packet.h"

void packet_send_entity_look_and_relative_move(struct bedrock_client *client, struct bedrock_client *targ, int8_t c_x, int8_t c_y, int8_t c_z, int8_t yaw, int8_t pitch)
{
	client_send_header(client, ENTITY_LOOK_AND_RELATIVE_MOVE);
	client_send_int(client, &targ->id, sizeof(targ->id));
	client_send_int(client, &c_x, sizeof(c_x));
	client_send_int(client, &c_y, sizeof(c_y));
	client_send_int(client, &c_z, sizeof(c_z));
	client_send_int(client, &yaw, sizeof(yaw));
	client_send_int(client, &pitch, sizeof(pitch));
}
