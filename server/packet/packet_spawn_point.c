#include "server/client.h"
#include "server/packet.h"
#include "nbt/nbt.h"

void packet_send_spawn_point(struct client *client)
{
	bedrock_packet packet;
	int32_t *spawn_x, *spawn_y, *spawn_z;
	struct position pos;

	spawn_x = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnX");
	spawn_y = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnY");
	spawn_z = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnZ");

	pos.x = *spawn_x;
	pos.y = *spawn_y;
	pos.z = *spawn_z;

	packet_init(&packet, SERVER_SPAWN_POINT);

	packet_pack_position(&packet, &pos);

	client_send_packet(client, &packet);
}
