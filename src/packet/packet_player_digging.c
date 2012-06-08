#include "server/client.h"
#include "server/packet.h"
#include "server/chunk.h"
#include "blocks/blocks.h"

int packet_player_digging(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint8_t status;
	int32_t x;
	uint8_t y;
	int32_t z;
	uint8_t face;

	packet_read_int(buffer, len, &offset, &status, sizeof(status));
	packet_read_int(buffer, len, &offset, &x, sizeof(x));
	packet_read_int(buffer, len, &offset, &y, sizeof(y));
	packet_read_int(buffer, len, &offset, &z, sizeof(z));
	packet_read_int(buffer, len, &offset, &face, sizeof(face));

	if (abs(*client_get_pos_x(client) - x) > 6 || abs(*client_get_pos_y(client) - y) > 6 || abs(*client_get_pos_z(client) - z) > 6)
		return ERROR_NOT_ALLOWED;

	{
		struct bedrock_chunk *chunk = find_chunk_which_contains(client->world, x, y, z);
		uint16_t block_index;
		struct bedrock_block *block;
		int32_t x2, z2;
		uint8_t y2;

		if (chunk == NULL)
			return ERROR_NOT_ALLOWED;

		chunk_decompress(chunk);

		printf("REAL COORDS: %d,%d,%d\n", x, y, z);

		x2 = x % BEDROCK_BLOCKS_PER_CHUNK;
		y2 = y % BEDROCK_BLOCKS_PER_CHUNK;
		z2 = z % BEDROCK_BLOCKS_PER_CHUNK;

		if (x2 < 0)
			x2 = BEDROCK_BLOCKS_PER_CHUNK - abs(x2);
		if (z2 < 0)
			z2 = BEDROCK_BLOCKS_PER_CHUNK - abs(z2);

		block_index = (y2 * 16 + z2) * 16 + x2;

		bedrock_assert(block_index < BEDROCK_BLOCK_LENGTH, return ERROR_NOT_ALLOWED);

		block = block_find_or_create(chunk->blocks[block_index]);
		printf("MINING BLOCK: %d,%d,%d which is %d\n", x, y, z, block->id);
		chunk->blocks[block_index] = 0;

		chunk_compress(chunk);

		bedrock_node *node;
		LIST_FOREACH(&client_list, node)
		{
			struct bedrock_client *c = node->data;
			client_send_header(c, 0x35);
			client_send_int(c, &x, sizeof(x));
			client_send_int(c, &y, sizeof(y));
			client_send_int(c, &z, sizeof(z));
			int8_t b = 0;
			client_send_int(c, &b, sizeof(b));
			client_send_int(c, &b, sizeof(b));
		}
	}

	return offset;
}
