#include "server/client.h"
#include "packet/packet.h"

void packet_send_spawn_named_entity(struct bedrock_client *client, struct bedrock_client *c)
{
	uint32_t abs_x, abs_y, abs_z;
	int8_t yaw, pitch;
	uint16_t item = 0; // XXX

	abs_x = ((int) *client_get_pos_x(c)) * 32;
	abs_y = ((int) *client_get_pos_y(c)) * 32;
	abs_z = ((int) *client_get_pos_z(c)) * 32;

	yaw = *client_get_yaw(c);
	pitch = *client_get_pitch(c);

	client_send_header(client, SPAWN_NAMED_ENTITY);
	client_send_int(client, &c->id, sizeof(c->id));
	client_send_string(client, c->name);
	client_send_int(client, &abs_x, sizeof(abs_x));
	client_send_int(client, &abs_y, sizeof(abs_y));
	client_send_int(client, &abs_z, sizeof(abs_z));
	client_send_int(client, &yaw, sizeof(yaw));
	client_send_int(client, &pitch, sizeof(pitch));
	client_send_int(client, &item, sizeof(item));
}
