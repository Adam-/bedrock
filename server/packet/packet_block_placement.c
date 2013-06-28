#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
#include "entities/entity.h"
#include "nbt/nbt.h"
#include "blocks/blocks.h"
#include "windows/window.h"
#include "packet/packet_block_change.h"
#include "packet/packet_open_window.h"

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

int packet_block_placement(struct client *client, const bedrock_packet *p)
{
	int offset = PACKET_HEADER_LENGTH;
	int32_t x, z;
	uint8_t y;
	uint8_t d;
	struct item_stack slot_data;
	uint8_t cursor_x, cursor_y, cursor_z;

	int32_t real_x, real_z;
	uint8_t real_y;

	struct item_stack *weilded_item;
	struct item *item;
	struct block *block;

	struct chunk *target_chunk, *real_chunk;
	uint8_t *placed_on, *being_placed;
	int32_t *height;

	bedrock_node *node;
	struct tile_entity *entity;

	packet_read_int(p, &offset, &x, sizeof(x));
	packet_read_int(p, &offset, &y, sizeof(y));
	packet_read_int(p, &offset, &z, sizeof(z));
	packet_read_int(p, &offset, &d, sizeof(d));
	packet_read_slot(p, &offset, &slot_data);
	packet_read_int(p, &offset, &cursor_x, sizeof(cursor_x));
	packet_read_int(p, &offset, &cursor_y, sizeof(cursor_y));
	packet_read_int(p, &offset, &cursor_z, sizeof(cursor_z));

	if (offset <= ERROR_UNKNOWN)
		return offset;

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

	if (abs(client->x - x) > 6 || abs(client->y - y) > 6 || abs(client->z - z) > 6)
		return ERROR_NOT_ALLOWED;

	// They are building onto a block in this chunk
	target_chunk = find_chunk_which_contains(client->world, x, y, z);
	if (target_chunk == NULL)
		return ERROR_UNEXPECTED;

	// They are building on to this block
	placed_on = chunk_get_block(target_chunk, x, y, z);
	if (placed_on == NULL)
		return ERROR_NOT_ALLOWED;

	weilded_item = &client->inventory[INVENTORY_HOTBAR_START + client->selected_slot];
	item = item_find_or_create(weilded_item->id);

	if (*placed_on == BLOCK_AIR)
	{
		/* Building on air, reject */
		bedrock_log(LEVEL_DEBUG, "player building: Rejecting block placement for %s because they are building on air at %d, %d, %d", client->name, real_x, real_y, real_z);
		client_add_inventory_item(client, item);
		packet_send_block_change(client, real_x, real_y, real_z, BLOCK_AIR, 0);
		return offset;
	}
	
	entity = column_find_tile_entity(target_chunk->column, ITEM_NONE, x, y, z);
	if (entity != NULL)
	{
		bedrock_log(LEVEL_DEBUG, "player building: %s operates entity %s at %d, %d, %d", client->name, item_find_or_create(entity->blockid)->name, real_x, real_y, real_z);
		entity_operate(client, entity);
		return offset;
	}

	bedrock_log(LEVEL_DEBUG, "player building: %s is building on %s with %s", client->name, item_find_or_create(*placed_on)->name, item->name);

	/* Not holding an item? */
	if (weilded_item->count == 0)
	{
		if (slot_data.id != -1)
			return ERROR_UNEXPECTED;
		else
			return offset;
	}

	if ((item->flags & ITEM_FLAG_BLOCK) == 0)
	{
		bedrock_log(LEVEL_DEBUG, "player building: %s is trying to place unknown or non-placeable block %d at %d,%d,%d, direction %d", client->name, weilded_item->id, x, y, z, d);

		client_add_inventory_item(client, item);
		packet_send_block_change(client, real_x, real_y, real_z, BLOCK_AIR, 0);

		return offset;
	}

	bedrock_log(LEVEL_DEBUG, "player building: %s is placing block of type %s at %d,%d,%d, direction %d", client->name, item->name, x, y, z, d);

	real_chunk = find_chunk_which_contains(client->world, real_x, real_y, real_z);
	if (real_chunk == NULL)
	{
		struct column *col;

		// If we can't find the new chunk it must be an up/down direction, not sideways
		bedrock_assert(x == real_x && z == real_z, return ERROR_NOT_ALLOWED);

		col = find_column_which_contains(find_region_which_contains(client->world, real_x, real_z), real_x, real_z);
		bedrock_assert(col != NULL, return ERROR_NOT_ALLOWED);

		bedrock_assert(real_y / BEDROCK_BLOCKS_PER_CHUNK < BEDROCK_BLOCKS_PER_CHUNK, return ERROR_UNEXPECTED);
		bedrock_assert(col->chunks[real_y / BEDROCK_BLOCKS_PER_CHUNK] == NULL, return ERROR_UNEXPECTED);

		real_chunk = chunk_create(col, real_y);
	}

	/* The new block wants to be placed here */
	being_placed = chunk_get_block(real_chunk, real_x, real_y, real_z);

	if (being_placed == NULL || *being_placed != BLOCK_AIR)
	{
		/* Being placed where something already is, reject */
		bedrock_log(LEVEL_DEBUG, "player building: Rejecting block placement for %s because a block is already at %d, %d, %d", client->name, real_x, real_y, real_z);
		client_add_inventory_item(client, item);
		packet_send_block_change(client, real_x, real_y, real_z, being_placed != NULL ? *being_placed : BLOCK_AIR, 0);
		return offset;
	}

	// This is the height right *above* the highest block
	height = column_get_height_for(real_chunk->column, real_x, real_z);
	if (real_y >= *height)
	{
		*height = real_y == 255 ? real_y : real_y + 1;
		bedrock_log(LEVEL_DEBUG, "player building: Adjusting height map of %d,%d to %d", real_x, real_z, *height);
	}

	*being_placed = slot_data.id;
	real_chunk->modified = true;
	column_set_pending(real_chunk->column, COLUMN_FLAG_DIRTY);

	/* Build has now succeeded to us, remove an item from the player
	 * if not in creative mode
	 */
	if (client->gamemode != GAMEMODE_CREATIVE)
	{
		weilded_item->count -= 1;
		if (!weilded_item->count)
		{
			weilded_item->id = 0;
			weilded_item->metadata = 0;
		}
	}

	// Notify clients who can see this column of the change
	LIST_FOREACH(&real_chunk->column->players, node)
	{
		struct client *c = node->data;
		packet_send_block_change(c, real_x, real_y, real_z, slot_data.id, 0);
	}

	block = block_find(slot_data.id);
	if (block != NULL && block->on_place != NULL)
		block->on_place(client, target_chunk, real_x, real_y, real_z, block);

	return offset;
}
