#include "packet/packet.h"
#include "nbt/nbt.h"

int packet_keep_alive(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	uint32_t id;

	packet_read_int(buffer, len, &offset, &id, sizeof(id));

	client_send_header(client, KEEP_ALIVE);
	client_send_int(client, &id, sizeof(id));

	bedrock_assert_ret(offset == 5, ERROR_UNKNOWN);
	return offset;
}

#include "server/region.h" // XXX
#include "compression/compression.h" // XXX
int packet_login_request(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	int32_t version, i;
	char string[BEDROCK_USERNAME_MAX];
	int8_t b;
	static uint32_t entity_id = 0;

	packet_read_int(buffer, len, &offset, &version, sizeof(version));
	packet_read_string(buffer, len, &offset, string, sizeof(string)); /* Username is sent here too, why? */
	packet_read_string(buffer, len, &offset, string, sizeof(string));
	packet_read_int(buffer, len, &offset, &i, sizeof(i));
	packet_read_int(buffer, len, &offset, &i, sizeof(i));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));
	packet_read_int(buffer, len, &offset, &b, sizeof(b));

	if (offset == ERROR_EAGAIN)
		return ERROR_EAGAIN;

	// Check version, should be 29

	client_send_header(client, LOGIN_REQUEST);
	++entity_id;
	client_send_int(client, &entity_id, sizeof(entity_id)); /* Entity ID */
	client_send_string(client, "");
	client_send_string(client, nbt_read_string(client->world->data, 2, "Data", "generatorName")); /* Generator name */
	client_send_int(client, nbt_read(client->world->data, TAG_INT, 2, "Data", "GameType"), sizeof(uint32_t)); /* Game type */
	client_send_int(client, nbt_read(client->data, TAG_INT, 1, "Dimension"), sizeof(uint32_t)); /* Dimension */
	client_send_int(client, nbt_read(client->world->data, TAG_BYTE, 2, "Data", "hardcore"), sizeof(uint8_t)); /* hardcore */
	b = 0;
	client_send_int(client, &b, sizeof(b));
	b = 8;
	client_send_int(client, &b, sizeof(b)); /* Max players */

	client->authenticated = STATE_BURSTING;

	// Time
	client_send_header(client, 0x04);
	uint64_t l = 6000;
	client_send_int(client, &l, sizeof(l));

	// Health
	client_send_header(client, 0x08);
	uint16_t s = 20;
	client_send_int(client, &s, sizeof(s));
	client_send_int(client, &s, sizeof(s));
	float f = 5;
	client_send_int(client, &f, sizeof(f));

	bedrock_node *node;
	LIST_FOREACH(&client->world->regions, node)
	{
		bedrock_region *region = node->data;

		printf("Loop regions, this one is %d %d\n", region->x, region->z);
		// Map Column Allocation (0x32)
		/*client_send_header(client, 0x32);
		client_send_int(client, &region->x, sizeof(region->x));
		client_send_int(client, &region->z, sizeof(region->z));
		b = 1;
		client_send_int(client, &b, sizeof(b));*/

		bedrock_node *node2;
		LIST_FOREACH(&region->columns, node2)
		{
			nbt_tag *tag = node2->data;

			printf("Looping columns? Packing %d %d\n", nbt_get(tag, 2, "Level", "xPos")->payload.tag_int, nbt_get(tag, 2, "Level", "zPos")->payload.tag_int);
			// DOES THIS GO HERE?
			// Map Column Allocation (0x32)
			client_send_header(client, 0x32);
			//client_send_int(client, &region->x, sizeof(uint32_t));
			//client_send_int(client, &region->z, sizeof(uint32_t));
			client_send_int(client, nbt_read(tag, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
			client_send_int(client, nbt_read(tag, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
			b = 1;
			client_send_int(client, &b, sizeof(b));

			// Map Chunks (0x33)
			client_send_header(client, 0x33);
			client_send_int(client, nbt_read(tag, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
			client_send_int(client, nbt_read(tag, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
			b = 0;
			client_send_int(client, &b, sizeof(b)); // Ground up continuous?

			uint16_t bitmask = 0;
			nbt_tag *sections = nbt_get(tag, 2, "Level", "Sections");
			bedrock_assert_ret(sections != NULL && sections->type == TAG_LIST, ERROR_UNKNOWN);
			bedrock_node *node3;
			LIST_FOREACH(&sections->payload.tag_compound, node3)
			{
				nbt_tag *sec = node3->data;

				nbt_copy(sec, &b, sizeof(b), 1, "Y");
				bitmask |= 1 << b;
				printf("Column contains chunk %d bitmask is now %d\n", b, bitmask);
			}
			client_send_int(client, &bitmask, sizeof(bitmask)); // primary bit map

			bitmask = 0; // ???????
			client_send_int(client, &bitmask, sizeof(bitmask)); // add bit map

			compression_buffer *buf = NULL;

			LIST_FOREACH(&sections->payload.tag_compound, node3)
			{
				nbt_tag *sec = node3->data;

				struct nbt_tag_byte_array *blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "Blocks");
				bedrock_assert_ret(blocks->length == 4096, ERROR_UNKNOWN);
				compression_compress(&buf, blocks->data, blocks->length);
				bedrock_assert_ret(buf != NULL, ERROR_UNKNOWN);
			}

			LIST_FOREACH(&sections->payload.tag_compound, node3)
			{
				nbt_tag *sec = node3->data;

				struct nbt_tag_byte_array *blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "Data");
				bedrock_assert_ret(blocks->length == 2048, ERROR_UNKNOWN);
				compression_compress(&buf, blocks->data, blocks->length);
				bedrock_assert_ret(buf != NULL, ERROR_UNKNOWN);
			}

			LIST_FOREACH(&sections->payload.tag_compound, node3)
			{
				nbt_tag *sec = node3->data;

				struct nbt_tag_byte_array *blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "BlockLight");
				bedrock_assert_ret(blocks->length == 2048, ERROR_UNKNOWN);
				compression_compress(&buf, blocks->data, blocks->length);
				bedrock_assert_ret(buf != NULL, ERROR_UNKNOWN);
			}

			LIST_FOREACH(&sections->payload.tag_compound, node3)
			{
				nbt_tag *sec = node3->data;

				struct nbt_tag_byte_array *blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "SkyLight");
				bedrock_assert_ret(blocks->length == 2048, ERROR_UNKNOWN);
				compression_compress(&buf, blocks->data, blocks->length);
				bedrock_assert_ret(buf != NULL, ERROR_UNKNOWN);
			}

			/*
			struct nbt_tag_byte_array *blocks = nbt_read(tag, TAG_BYTE_ARRAY, 2, "Level", "Biomes");
			bedrock_assert_ret(blocks->length == 256, ERROR_UNKNOWN);
			compression_compress(&buf, blocks->data, blocks->length);
			bedrock_assert_ret(buf != NULL, ERROR_UNKNOWN);
			*/

			uint32_t ii = buf->length;
			client_send_int(client, &ii, sizeof(ii));
			ii = 0;
			client_send_int(client, &ii, sizeof(ii));

			client_send(client, buf->data, buf->length);
			compression_free_buffer(buf);
		}
	}

	// Spawn Position (0x06)
	client_send_header(client, 0x06);
	client_send_int(client, nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnX"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnY"), sizeof(uint32_t)); // Y
	client_send_int(client, nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnZ"), sizeof(uint32_t)); // Z

	// Player Position & Look (0x0D)
	client_send_header(client, 0x0D);
	double d = 0;
	client_send_int(client, &d, sizeof(d)); // X
	//client_send_int(client, nbt_read(client->data, TAG_DOUBLE, 2, "Pos", 0), sizeof(double)); // X?
	client_send_int(client, &d, sizeof(d)); // Stance
	d = 50;
	client_send_int(client, &d, sizeof(d)); // Y
	d = 0;
	client_send_int(client, &d, sizeof(d)); // Z
	//client_send_int(client, nbt_read(client->data, TAG_DOUBLE, 2, "Pos", 1), sizeof(double)); // Y?
	//client_send_int(client, nbt_read(client->data, TAG_DOUBLE, 2, "Pos", 2), sizeof(double)); // Z?
	f = 0;
	client_send_int(client, &f, sizeof(f)); // Yaw
	client_send_int(client, &f, sizeof(f)); // Pitch
	b = 1;
	client_send_int(client, &b, sizeof(b)); // On ground

	// Entity Equipment (0x05) 1
	client_send_header(client, 0x05);
	client_send_int(client, &entity_id, sizeof(entity_id));
	s = 0;
	client_send_int(client, &s, sizeof(s));
	s = -1;
	client_send_int(client, &s, sizeof(s));
	s = 0;
	client_send_int(client, &s, sizeof(s));

	// 2
	client_send_header(client, 0x05);
	client_send_int(client, &entity_id, sizeof(entity_id));
	s = 1;
	client_send_int(client, &s, sizeof(s));
	s = -1;
	client_send_int(client, &s, sizeof(s));
	s = 0;
	client_send_int(client, &s, sizeof(s));

	// 3
	client_send_header(client, 0x05);
	client_send_int(client, &entity_id, sizeof(entity_id));
	s = 2;
	client_send_int(client, &s, sizeof(s));
	s = -1;
	client_send_int(client, &s, sizeof(s));
	s = 0;
	client_send_int(client, &s, sizeof(s));

	// 4
	client_send_header(client, 0x05);
	client_send_int(client, &entity_id, sizeof(entity_id));
	s = 3;
	client_send_int(client, &s, sizeof(s));
	s = -1;
	client_send_int(client, &s, sizeof(s));
	s = 0;
	client_send_int(client, &s, sizeof(s));

	// 5
	client_send_header(client, 0x05);
	client_send_int(client, &entity_id, sizeof(entity_id));
	s = 4;
	client_send_int(client, &s, sizeof(s));
	s = -1;
	client_send_int(client, &s, sizeof(s));
	s = 0;
	client_send_int(client, &s, sizeof(s));

	// set slot?
	//client_send_header(client, 0x67);
	//b = 0;
	//client_send_int(client, &b, sizeof(b)); // window, 0 = inv

	return offset;
}

int packet_handshake(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	bedrock_world *world;
	char username[BEDROCK_USERNAME_MAX + 1 + 64 + 1 + 5];
	char *p;

	packet_read_string(buffer, len, &offset, username, sizeof(username));

	if (offset == ERROR_EAGAIN)
		return ERROR_EAGAIN;

	p = strchr(username, ';');
	if (p == NULL)
		return ERROR_INVALID_FORMAT;

	*p = 0;

	if (client_valid_username(username) == false)
		return ERROR_INVALID_FORMAT;

	// Check already exists

	world = world_find(BEDROCK_WORLD_NAME);
	bedrock_assert_ret(world != NULL, ERROR_UNKNOWN);

	strncpy(client->name, username, sizeof(client->name));
	client->authenticated = STATE_HANDSHAKING;
	client->world = world;

	client_load(client); // Can fail if a new client
	assert(client->data); // XXX

	client_send_header(client, HANDSHAKE);
	client_send_string(client, "-");

	return offset;
}

int packet_position(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	double x, y, stance, z;
	uint8_t on_ground;

	packet_read_int(buffer, len, &offset, &x, sizeof(x));
	packet_read_int(buffer, len, &offset, &y, sizeof(y));
	packet_read_int(buffer, len, &offset, &stance, sizeof(stance));
	packet_read_int(buffer, len, &offset, &z, sizeof(z));
	packet_read_int(buffer, len, &offset, &on_ground, sizeof(on_ground));

	bedrock_assert_ret(offset == 34, ERROR_INVALID_FORMAT);
	return offset;
}

int packet_position_and_look(bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	double x, y, stance, z;
	float yaw, pitch;
	uint8_t on_ground;

	assert(sizeof(double) == 8);
	assert(sizeof(float) == 4);

	packet_read_int(buffer, len, &offset, &x, sizeof(x));
	packet_read_int(buffer, len, &offset, &y, sizeof(y));
	packet_read_int(buffer, len, &offset, &stance, sizeof(stance));
	packet_read_int(buffer, len, &offset, &z, sizeof(z));
	packet_read_int(buffer, len, &offset, &yaw, sizeof(yaw));
	packet_read_int(buffer, len, &offset, &pitch, sizeof(pitch));
	packet_read_int(buffer, len, &offset, &on_ground, sizeof(on_ground));

	bedrock_assert_ret(offset == 42, ERROR_INVALID_FORMAT);
	return offset;
}
