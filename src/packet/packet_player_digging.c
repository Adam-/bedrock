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

static uint8_t *get_block_from_chunk(struct bedrock_chunk *chunk, int32_t x, uint8_t y, int32_t z)
{
	uint16_t block_index;

	x %= BEDROCK_BLOCKS_PER_CHUNK;
	y %= BEDROCK_BLOCKS_PER_CHUNK;
	z %= BEDROCK_BLOCKS_PER_CHUNK;

	if (x < 0)
		x = BEDROCK_BLOCKS_PER_CHUNK - abs(x);
	if (z < 0)
		z = BEDROCK_BLOCKS_PER_CHUNK - abs(z);

	block_index = (y * BEDROCK_BLOCKS_PER_CHUNK + z) * BEDROCK_BLOCKS_PER_CHUNK + x;

	bedrock_assert(block_index < BEDROCK_BLOCK_LENGTH, return NULL);

	return &chunk->blocks[block_index];
}

static struct bedrock_item *get_weilded_item(struct bedrock_client *client)
{
	nbt_tag *tag = client_get_inventory_tag(client, client->selected_slot);
	uint16_t *id = nbt_read(tag, TAG_SHORT, 1, "id");
	return item_find_or_create(*id);
}

static double modulus(double x, double y)
{
	return x - (double) (int) (x / y) * y;
}

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
		uint8_t *block_id;
		struct bedrock_block *block;
		double delay;
		struct bedrock_item *item = get_weilded_item(client);

		// Reset dig state
		memset(&client->digging_data, 0, sizeof(client->digging_data));

		if (chunk == NULL)
			return ERROR_NOT_ALLOWED;

		chunk_decompress(chunk);

		block_id = get_block_from_chunk(chunk, x, y, z);
		if (block_id == NULL)
		{
			chunk_compress(chunk);
			return ERROR_NOT_ALLOWED;
		}

		block = block_find_or_create(*block_id);

		bedrock_log(LEVEL_DEBUG, "player digging: %s is digging coords %d,%d,%d which is of type %s", client->name, x, y, z, block->name);

		if (block->requirement != ITEM_FLAG_NONE)
		{
			bedrock_assert((block->requirement & ~(TOOL_NAME_MASK | TOOL_TYPE_MASK)) == 0, ;);

			// Extract tool name from block requirement, if one exists, and see if the player has a tool of that type. if there is no tool requirement anything goes.
			bool has_tool_requirement = block->requirement & TOOL_NAME_MASK ? block->requirement & item->flags & TOOL_NAME_MASK : true,
					has_type_requirement = block->requirement & TOOL_TYPE_MASK ? block->requirement & item->flags & TOOL_TYPE_MASK : true;

			if (!has_tool_requirement || !has_type_requirement)
			{
				bedrock_log(LEVEL_DEBUG, "player digging: %s does not have the required tool to mine this block", client->name);

				chunk_compress(chunk);
				return offset;
			}
		}

		delay = block->hardness;

		if (block->weakness != ITEM_FLAG_NONE)
		{
			bedrock_assert((block->weakness & ~TOOL_NAME_MASK) == 0, ;);

			if (block->weakness & item->flags)
			{
				// http://www.minecraftwiki.net/wiki/Digging
				if (block->weakness & item->flags & ITEM_FLAG_GOLD)
					delay /= 12;
				else if (block->weakness & item->flags & ITEM_FLAG_DIAMOND)
					delay /= 8;
				else if (block->weakness & item->flags & ITEM_FLAG_IRON)
					delay /= 6;
				else if (block->weakness & item->flags & ITEM_FLAG_STONE)
					delay /= 4;
				else if (block->weakness & item->flags & ITEM_FLAG_WOOD)
					delay /= 2;
			}
		}

		client->digging_data.x = x;
		client->digging_data.y = y;
		client->digging_data.z = z;
		client->digging_data.block_id = block->id;
		client->digging_data.item_id = item->id;
		client->digging_data.end.tv_sec = bedrock_time.tv_sec + delay / 1;
		client->digging_data.end.tv_nsec = bedrock_time.tv_nsec + modulus(delay, 1.0);
		if (client->digging_data.end.tv_nsec >= 1000000000)
		{
			++client->digging_data.end.tv_sec;
			client->digging_data.end.tv_nsec -= 1000000000;
		}

		chunk_compress(chunk);
	}
	else if (status == FINISHED_DIGGING)
	{
		struct bedrock_item *item = get_weilded_item(client);
		struct bedrock_chunk *chunk;
		uint8_t *block_id;
		struct bedrock_block *block;
		bedrock_node *node;
		int i;

		chunk = find_chunk_which_contains(client->world, x, y, z);
		if (chunk == NULL)
			return ERROR_NOT_ALLOWED;

		chunk_decompress(chunk);

		block_id = get_block_from_chunk(chunk, x, y, z);
		if (block_id == NULL)
		{
			chunk_compress(chunk);
			return ERROR_NOT_ALLOWED;
		}

		if (x != client->digging_data.x || y != client->digging_data.y || z != client->digging_data.z || client->digging_data.end.tv_sec == 0 || client->digging_data.item_id != item->id || client->digging_data.block_id != *block_id)
		{
			bedrock_log(LEVEL_DEBUG, "player digging: Mismatch in dig data - saved: X: %d Y: %d Z: %d T: %d I: %d B: %d - got: X: %d Y: %d Z: %d I: %d B: %d",
					client->digging_data.x, client->digging_data.y, client->digging_data.z, client->digging_data.end.tv_sec, client->digging_data.item_id, client->digging_data.block_id,
					x, y, z, item->id, *block_id);

			packet_send_block_change(client, x, y, z, *block_id, 0);
			chunk_compress(chunk);
			return offset;
		}

		if (bedrock_time.tv_sec < client->digging_data.end.tv_sec || (bedrock_time.tv_sec == client->digging_data.end.tv_sec && bedrock_time.tv_nsec < client->digging_data.end.tv_nsec))
		{
			packet_send_block_change(client, x, y, z, *block_id, 0);
			chunk_compress(chunk);
			return offset;
		}

		*block_id = BLOCK_AIR;
		chunk->modified = true;

		// Notify players in render distance of the column to remove the block
		LIST_FOREACH(&chunk->column->players, node)
		{
			struct bedrock_client *c = node->data;

			packet_send_block_change(c, x, y, z, BLOCK_AIR, 0);
		}

		block = block_find_or_create(*block_id);
		if (block->on_mine == NULL)
			;
		else if (block->reap != ITEM_FLAG_NONE)
		{
			bedrock_assert((block->reap & ~(TOOL_NAME_MASK | TOOL_TYPE_MASK)) == 0, ;);

			// Extract tool name from block requirement, if one exists, and see if the player has a tool of that type. if there is no tool requirement anything goes.
			bool has_tool_requirement = block->reap & TOOL_NAME_MASK ? block->reap & item->flags & TOOL_NAME_MASK : true,
					has_type_requirement = block->reap & TOOL_TYPE_MASK ? block->reap & item->flags & TOOL_TYPE_MASK : true;

			if (has_tool_requirement && has_type_requirement)
				block->on_mine(client, block);
		}
		else
			block->on_mine(client, block);

		// If the block is all air delete it;
		for (i = 0; i < BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK; ++i)
			if (chunk->blocks[i] != BLOCK_AIR)
				break;
		if (i == BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK)
			chunk_free(chunk);
		else
			chunk_compress(chunk);
	}
	else
		bedrock_log(LEVEL_DEBUG, "player digging: Unrecognized dig status %d", status);

	return offset;
}
