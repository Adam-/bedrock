#include "server/client.h"
#include "server/packet.h"
#include "nbt/nbt.h"

void packet_send_spawn_point(struct client *client)
{
	bedrock_packet packet;
	int32_t *spawn_x, *spawn_y, *spawn_z;

	spawn_x = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnX");
	spawn_y = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnY");
	spawn_z = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnZ");

	packet_init(&packet, SPAWN_POINT);

	packet_pack_header(&packet, SPAWN_POINT);
	packet_pack_int(&packet, spawn_x, sizeof(*spawn_x));
	packet_pack_int(&packet, spawn_y, sizeof(*spawn_y));
	packet_pack_int(&packet, spawn_z, sizeof(*spawn_z));

	client_send_packet(client, &packet);
}
