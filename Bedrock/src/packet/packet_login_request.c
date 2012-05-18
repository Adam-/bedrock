#include "server/client.h"
#include "packet/packet.h"
#include "nbt/nbt.h"
#include "packet/packet_spawn_point.h"
#include "packet/packet_disconnect.h"

int packet_login_request(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	int32_t version, i;
	char username[BEDROCK_USERNAME_MAX], unused[1];
	int8_t b;
	int32_t *spawn_x, *spawn_y, *spawn_z;

	packet_read_int(buffer, len, &offset, &version, sizeof(version));
	packet_read_string(buffer, len, &offset, username, sizeof(username)); /* Username is sent here too, why? */
	packet_read_string(buffer, len, &offset, unused, sizeof(unused));
	packet_read_int(buffer, len, &offset, &i, sizeof(i));
	packet_read_int(buffer, len, &offset, &i, sizeof(i));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));

	if (offset == ERROR_EAGAIN)
		return ERROR_EAGAIN;

	// Check version, should be 29
	if (version != BEDROCK_PROTOCOL_VERSION)
	{
		packet_send_disconnect(client, "Incorrect version");
		return offset;
	}
	else if (strcmp(client->name, username))
	{
		packet_send_disconnect(client, "Username mismatch");
		return offset;
	}

	client_send_header(client, LOGIN_REQUEST);
	client_send_int(client, &entity_id, sizeof(entity_id)); /* Entity ID */
	++entity_id;
	client_send_string(client, "");
	client_send_string(client, nbt_read_string(client->world->data, 2, "Data", "generatorName")); /* Generator name */
	client_send_int(client, nbt_read(client->world->data, TAG_INT, 2, "Data", "GameType"), sizeof(uint32_t)); /* Game type */
	client_send_int(client, nbt_read(client->data, TAG_INT, 1, "Dimension"), sizeof(uint32_t)); /* Dimension */
	client_send_int(client, nbt_read(client->world->data, TAG_BYTE, 2, "Data", "hardcore"), sizeof(uint8_t)); /* hardcore */
	b = 0;
	client_send_int(client, &b, sizeof(b)); /* Not used */
	b = 8;
	client_send_int(client, &b, sizeof(b)); /* Max players */

	client->authenticated = STATE_BURSTING;

	spawn_x = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnX");
	spawn_y = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnY");
	spawn_z = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnZ");
	packet_send_spawn_point(client, *spawn_x, *spawn_y, *spawn_z);

	client_update_chunks(client);

	packet_send_player_and_look(client);

	return offset;
}
