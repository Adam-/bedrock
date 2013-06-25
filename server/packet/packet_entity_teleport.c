#include "server/client.h"
#include "server/packet.h"

void packet_send_entity_teleport(struct client *client, struct client *targ)
{
	bedrock_packet packet;
	int32_t a_x, a_y, a_z;
	int8_t new_y, new_p;

	a_x = targ->x * 32;
	a_y = targ->y * 32;
	a_z = targ->z * 32;

	new_y = (targ->yaw / 360.0) * 256;
	new_p = (targ->pitch / 360.0) * 256;

	packet_init(&packet, ENTITY_TELEPORT);

	packet_pack_header(&packet, ENTITY_TELEPORT);
	packet_pack_int(&packet, &targ->id, sizeof(targ->id));
	packet_pack_int(&packet, &a_x, sizeof(a_x));
	packet_pack_int(&packet, &a_y, sizeof(a_y));
	packet_pack_int(&packet, &a_z, sizeof(a_z));
	packet_pack_int(&packet, &new_y, sizeof(new_y));
	packet_pack_int(&packet, &new_p, sizeof(new_p));

	client_send_packet(client, &packet);
}
