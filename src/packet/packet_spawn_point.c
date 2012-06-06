#include "server/client.h"
#include "packet/packet.h"

void packet_send_spawn_point(struct bedrock_client *client)
{
	int32_t *spawn_x, *spawn_y, *spawn_z;

	spawn_x = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnX");
	spawn_y = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnY");
	spawn_z = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnZ");

	client_send_header(client, SPAWN_POINT);
	client_send_int(client, spawn_x, sizeof(*spawn_x));
	client_send_int(client, spawn_y, sizeof(*spawn_y));
	client_send_int(client, spawn_z, sizeof(*spawn_z));
}
