#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
#include "nbt/nbt.h"
#include "blocks/blocks.h"
#include "packet/packet_block_change.h"

enum
{
	DOWN_Y,
	UP_Y,
	DOWN_Z,
	UP_Z,
	DOWN_X,
	UP_X,
	UPDATE = 0xFF
};

int packet_block_placement(struct bedrock_client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	int32_t x, z;
	uint8_t y;
	uint8_t d;
	int16_t id;
	uint8_t count = 0;
	int16_t metadata = 0;

	nbt_tag *weilded_item;
	struct bedrock_item *item;

	struct bedrock_chunk *target_chunk, *real_chunk;
	int32_t *height;

	int32_t real_x, real_z;
	uint8_t real_y;

	packet_read_int(p, &offset, &x, sizeof(x));
	packet_read_int(p, &offset, &y, sizeof(y));
	packet_read_int(p, &offset, &z, sizeof(z));
	packet_read_int(p, &offset, &d, sizeof(d));
	packet_read_int(p, &offset, &id, sizeof(id));
	if (id != -1)
	{
		packet_read_int(p, &offset, &count, sizeof(count));
		packet_read_int(p, &offset, &metadata, sizeof(metadata));
	}

	real_x = x;
	real_y = y;
	real_z = z;

	switch (d)
	{
		case DOWN_Y:
			if (real_y == 0)
				return ERROR_NOT_ALLOWED;
			--real_y;
			break;
		case UP_Y:
			if (real_y == (uint8_t) ~0)
				return ERROR_NOT_ALLOWED;
			++real_y;
			break;
		case DOWN_Z:
			if (real_z == 0)
				return ERROR_NOT_ALLOWED;
			--real_z;
			break;
		case UP_Z:
			if (real_z == ~0)
				return ERROR_NOT_ALLOWED;
			++real_z;
			break;
		case DOWN_X:
			if (real_x == 0)
				return ERROR_NOT_ALLOWED;
			--real_x;
			break;
		case UP_X:
			if (real_x == ~0)
				return ERROR_NOT_ALLOWED;
			++real_x;
			break;
		case UPDATE:
			// This packet has a special case where X, Y, Z, and Direction are all -1.
			// This special packet indicates that the currently held item for the player should have its state
			// updated such as eating food, shooting bows, using buckets, etc.
			return offset;
		default: // Unknown direction
			return ERROR_NOT_ALLOWED;
	}

	weilded_item = client_get_inventory_tag(client, client->selected_slot);
	if (weilded_item == NULL)
	{
		if (id != -1)
			return ERROR_UNEXPECTED;
		else
			return offset;
	}
	else
	{
		int16_t *weilded_id = nbt_read(weilded_item, TAG_SHORT, 1, "id");
		uint8_t *weilded_count = nbt_read(weilded_item, TAG_BYTE, 1, "Count");


		// At this point the client has already removed one
		*weilded_count -= 1;

		item = item_find(*weilded_id);
		if (item == NULL || (item->flags & ITEM_FLAG_BLOCK) == 0)
		{
			bedrock_log(LEVEL_DEBUG, "player building: %s is trying to place unknown or non-placeable block %d at %d,%d,%d, direction %d", client->name, *weilded_id, x, y, z, d);

			if (*weilded_count == 0)
				nbt_free(weilded_item);

			return offset;
		}

		bedrock_log(LEVEL_DEBUG, "player building: %s is placing block of type %s at %d,%d,%d, direction %d", client->name, item->name, x, y, z, d);

		if (*weilded_count == 0)
			nbt_free(weilded_item);
	}

	if (abs(*client_get_pos_x(client) - x) > 6 || abs(*client_get_pos_y(client) - y) > 6 || abs(*client_get_pos_z(client) - z) > 6)
		return ERROR_NOT_ALLOWED;

	// They are building onto a block in this chunk
	target_chunk = find_chunk_which_contains(client->world, x, y, z);
	if (target_chunk == NULL)
		return ERROR_UNEXPECTED;
	else
	{
		uint8_t *placed_on;

		chunk_decompress(target_chunk);

		placed_on = chunk_get_block(target_chunk, x, y, z);
		if (placed_on == NULL)
			return ERROR_NOT_ALLOWED;
		else if (*placed_on == BLOCK_AIR)
		{
			client_add_inventory_item(client, item);
			packet_send_block_change(client, real_x, real_y, real_z, BLOCK_AIR, 0);
			return offset;
		}
	}

	real_chunk = find_chunk_which_contains(client->world, real_x, real_y, real_z);
	if (real_chunk == NULL)
	{
		struct bedrock_column *col;

		// If we can't find the new chunk it must be an up/down direction, not sideways
		bedrock_assert(x == real_x && z == real_z, return ERROR_NOT_ALLOWED);

		col = find_column_which_contains(find_region_which_contains(client->world, real_x, real_z), real_x, real_z);
		bedrock_assert(col != NULL, return ERROR_NOT_ALLOWED);

		bedrock_assert(real_y / BEDROCK_BLOCKS_PER_CHUNK < BEDROCK_BLOCKS_PER_CHUNK, return ERROR_UNEXPECTED);
		bedrock_assert(col->chunks[real_y / BEDROCK_BLOCKS_PER_CHUNK] == NULL, return ERROR_UNEXPECTED);

		real_chunk = chunk_create(col, real_y);
	}
	else
	{
		uint8_t *being_placed;

		chunk_decompress(real_chunk);

		being_placed = chunk_get_block(real_chunk, real_x, real_y, real_z);

		if (being_placed == NULL || *being_placed != BLOCK_AIR)
		{
			client_add_inventory_item(client, item);
			packet_send_block_change(client, real_x, real_y, real_z, being_placed != NULL ? *being_placed : BLOCK_AIR, 0);
			return offset;
		}
	}

	{
		uint8_t *being_placed;
		bedrock_node *node;

		chunk_decompress(real_chunk);

		being_placed = chunk_get_block(real_chunk, real_x, real_y, real_z);
		bedrock_assert(being_placed != NULL, return offset);

		// This is the height right *above* the highest block
		height = column_get_height_for(real_chunk->column, real_x, real_z);
		if (real_y >= *height)
		{
			*height = real_y == 255 ? real_y : real_y + 1;
			bedrock_log(LEVEL_DEBUG, "player building: Adjusting height map of %d,%d to %d", real_x, real_z, *height);
		}

		*being_placed = id;
		real_chunk->modified = true;
		column_dirty(real_chunk->column);

		// Notify clients who can see this column of the change
		LIST_FOREACH(&real_chunk->column->players, node)
		{
			struct bedrock_client *c = node->data;
			packet_send_block_change(c, real_x, real_y, real_z, id, 0);
		}
	}

	chunk_compress(real_chunk);

	return offset;
}
