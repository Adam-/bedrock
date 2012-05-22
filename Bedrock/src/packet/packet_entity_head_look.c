#include "server/client.h"
#include "packet/packet.h"

void packet_send_entity_head_look(struct bedrock_client *client, struct bedrock_client *target)
{
	int8_t new_y = (*client_get_yaw(target) / 360.0) * 256;

	client_send_header(client, ENTITY_HEAD_LOOK);
	client_send_int(client, &target->id, sizeof(target->id));
	client_send_int(client, &new_y, sizeof(new_y));
}
