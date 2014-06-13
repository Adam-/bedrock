#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "server/packets.h"
#include "server/column.h"

void packet_spawn_object_item(struct client *client, struct dropped_item *di)
{
	bedrock_packet packet;
	int32_t a_x, a_y, a_z;
	uint8_t b;
	uint32_t i;

	packet_init(&packet, SERVER_SPAWN_OBJECT);

	packet_pack_varuint(&packet, di->p.id);

	b = 2;
	packet_pack_int(&packet, &b, sizeof(b));

	a_x = (int) di->p.pos.x * 32;
	a_y = (int) di->p.pos.y * 32;
	a_z = (int) di->p.pos.z * 32;

	packet_pack_int(&packet, &a_x, sizeof(a_x));
	packet_pack_int(&packet, &a_y, sizeof(a_y));
	packet_pack_int(&packet, &a_z, sizeof(a_z));

	b = 0;
	packet_pack_int(&packet, &b, sizeof(b));
	packet_pack_int(&packet, &b, sizeof(b));

	i = 0;
	packet_pack_int(&packet, &i, sizeof(i));

	client_send_packet(client, &packet);

	packet_send_entity_metadata_slot(client, di);
}
