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

		if (chunk == NULL)
			return ERROR_NOT_ALLOWED;

		chunk_decompress(chunk);

		x %= BEDROCK_BLOCKS_PER_CHUNK;
		y %= BEDROCK_BLOCKS_PER_CHUNK;
		z %= BEDROCK_BLOCKS_PER_CHUNK;

		block_index = (y * 16 + z) * 16 + x;

		bedrock_assert(block_index < BEDROCK_BLOCK_LENGTH, return ERROR_NOT_ALLOWED);

		block = block_find_or_create(chunk->blocks[block_index]);
		printf("MINING BLOCK: %d\n", block->id);

		chunk_compress(chunk);
	}

	return offset;
}
