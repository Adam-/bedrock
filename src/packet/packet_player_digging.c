#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
#include "blocks/blocks.h"
#include "nbt/nbt.h"

enum
{
	STARTED_DIGGING,
	FINISHED_DIGGING = 2
};

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

	if (status == STARTED_DIGGING)
	{
		struct bedrock_chunk *chunk = find_chunk_which_contains(client->world, x, y, z);
		uint16_t block_index;
		struct bedrock_block *block;
		int32_t x2, z2;
		uint8_t y2;
		double delay;
		//bedrock_node *node;

		if (chunk == NULL)
			return ERROR_NOT_ALLOWED;

		chunk_decompress(chunk);

		x2 = x % BEDROCK_BLOCKS_PER_CHUNK;
		y2 = y % BEDROCK_BLOCKS_PER_CHUNK;
		z2 = z % BEDROCK_BLOCKS_PER_CHUNK;

		if (x2 < 0)
			x2 = BEDROCK_BLOCKS_PER_CHUNK - abs(x2);
		if (z2 < 0)
			z2 = BEDROCK_BLOCKS_PER_CHUNK - abs(z2);

		block_index = (y2 * BEDROCK_BLOCKS_PER_CHUNK + z2) * BEDROCK_BLOCKS_PER_CHUNK + x2;

		bedrock_assert(block_index < BEDROCK_BLOCK_LENGTH, return ERROR_NOT_ALLOWED);

		block = block_find_or_create(chunk->blocks[block_index]);

		delay = block->hardness;

		if (block->requirement != TYPE_NONE)

		client->digging_data.x = x;
		client->digging_data.y = y;
		client->digging_data.z = z;
		client->digging_data.id = block->id;
		client->digging_data.end = block->hardness;

		//chunk->blocks[block_index] = BLOCK_AIR;
		//chunk->modified = true;

		/*{
			int i;
			for (i = 0; i < BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK; ++i)
				if (chunk->blocks[i] != BLOCK_AIR)
					break;
			if (i == BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK)
				chunk_free(chunk);
			else
				chunk_compress(chunk);
		}

		LIST_FOREACH(&chunk->column->players, node)
		{
			struct bedrock_client *c = node->data;

			packet_send_block_change(c, x, y, z, BLOCK_AIR, 0);
		}*/
	}

	return offset;
}
