#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
#include "blocks/blocks.h"
#include "nbt/nbt.h"
#include "packet/packet_block_change.h"
#include "packet/packet_set_slot.h"

enum
{
	STARTED_DIGGING,
	FINISHED_DIGGING = 2,
	DROP_ITEM = 4
};

static struct bedrock_item *get_weilded_item(struct bedrock_client *client)
{
	nbt_tag *tag = client_get_inventory_tag(client, client->selected_slot);
	if (tag != NULL)
	{
		uint16_t *id = nbt_read(tag, TAG_SHORT, 1, "id");
		if (id != NULL)
			return item_find_or_create(*id);
	}

	return item_find_or_create(ITEM_NONE);
}

static double modulus(double x, double y)
{
	return x - (double) (int) (x / y) * y;
}

/* Is the given item a weakness for the given block? */
static bool is_weakness(struct bedrock_block *block, struct bedrock_item *item)
{
	bool has_tool_requirement, has_type_requirement;

	bedrock_assert((block->weakness & ~(TOOL_NAME_MASK | TOOL_TYPE_MASK)) == 0, ;);

	// Extract tool name from block requirement, if one exists, and see if the player has a tool of that type. if there is no tool requirement anything goes.
	has_tool_requirement = block->weakness & TOOL_NAME_MASK ? block->weakness & item->flags & TOOL_NAME_MASK : true;
	// And the same for tool type requirement
	has_type_requirement = block->weakness & TOOL_TYPE_MASK ? block->weakness & item->flags & TOOL_TYPE_MASK : true;

	return has_tool_requirement && has_type_requirement;
}

/* Can the given item harvest the given block? */
static bool can_harvest(struct bedrock_block *block, struct bedrock_item *item)
{
	bool has_tool_requirement, has_type_requirement;

	bedrock_assert((block->harvest & ~(TOOL_NAME_MASK | TOOL_TYPE_MASK)) == 0, ;);

	// Extract tool name from block requirement, if one exists, and see if the player has a tool of that type. if there is no tool requirement anything goes.
	has_tool_requirement = block->harvest & TOOL_NAME_MASK ? block->harvest & item->flags & TOOL_NAME_MASK : true;
	// And the same for tool type requirement
	has_type_requirement = block->harvest & TOOL_TYPE_MASK ? block->harvest & item->flags & TOOL_TYPE_MASK : true;

	return has_tool_requirement && has_type_requirement;
}

/* Calculate how long a block should take to mine using the given item.
 * See: http://www.minecraftwiki.net/wiki/Digging
 */
static double calculate_block_time(struct bedrock_client bedrock_attribute_unused *client, struct bedrock_block *block, struct bedrock_item *item)
{
	// Start with the time, in seconds, it takes to mine the block for no harvest
	double delay = block->no_harvest_time;

	// If this block can be harvested by this item
	if (is_weakness(block, item))
	{
		// Set the delay to the hardness
		delay = block->hardness;

		// If the block has a weakness
		if (block->weakness != ITEM_FLAG_NONE)
		{
			bedrock_assert((block->weakness & ~(TOOL_NAME_MASK | TOOL_TYPE_MASK)) == 0, ;);

			// If our item matches one of the weaknesses
			if (block->weakness & item->flags & TOOL_NAME_MASK)
			{
				// Reduce delay accordingly
				if (item->flags & ITEM_FLAG_GOLD)
					delay /= 12;
				else if (item->flags & ITEM_FLAG_DIAMOND)
					delay /= 8;
				else if (item->flags & ITEM_FLAG_IRON)
					delay /= 6;
				else if (item->flags & ITEM_FLAG_STONE)
					delay /= 4;
				else if (item->flags & ITEM_FLAG_WOOD)
					delay /= 2;
			}
		}
	}

	return delay;
}

int packet_player_digging(struct bedrock_client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	uint8_t status;
	int32_t x;
	uint8_t y;
	int32_t z;
	uint8_t face;

	packet_read_int(p, &offset, &status, sizeof(status));
	packet_read_int(p, &offset, &x, sizeof(x));
	packet_read_int(p, &offset, &y, sizeof(y));
	packet_read_int(p, &offset, &z, sizeof(z));
	packet_read_int(p, &offset, &face, sizeof(face));

	if (status == STARTED_DIGGING)
	{
		struct bedrock_chunk *chunk = find_chunk_which_contains(client->world, x, y, z);
		uint8_t *block_id;
		struct bedrock_block *block;
		double delay;
		struct bedrock_item *item = get_weilded_item(client);

		if (abs(*client_get_pos_x(client) - x) > 6 || abs(*client_get_pos_y(client) - y) > 6 || abs(*client_get_pos_z(client) - z) > 6)
			return ERROR_NOT_ALLOWED;

		// Reset dig state
		memset(&client->digging_data, 0, sizeof(client->digging_data));

		if (chunk == NULL)
			return ERROR_NOT_ALLOWED;

		block_id = chunk_get_block(chunk, x, y, z);
		if (block_id == NULL)
			return ERROR_NOT_ALLOWED;

		block = block_find_or_create(*block_id);

		bedrock_log(LEVEL_DEBUG, "player digging: %s is digging coords %d,%d,%d which is of type %s", client->name, x, y, z, block->name);

		delay = calculate_block_time(client, block, item);

		// Special case, unmineable
		if (delay < 0)
			return offset;

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
	}
	else if (status == FINISHED_DIGGING)
	{
		struct bedrock_item *item = get_weilded_item(client);
		struct bedrock_chunk *chunk;
		uint8_t *block_id;
		struct bedrock_block *block;
		int32_t *height;
		bedrock_node *node;
		int i;

		if (abs(*client_get_pos_x(client) - x) > 6 || abs(*client_get_pos_y(client) - y) > 6 || abs(*client_get_pos_z(client) - z) > 6)
			return ERROR_NOT_ALLOWED;

		chunk = find_chunk_which_contains(client->world, x, y, z);
		if (chunk == NULL)
			return ERROR_NOT_ALLOWED;

		block_id = chunk_get_block(chunk, x, y, z);
		if (block_id == NULL)
			return ERROR_NOT_ALLOWED;

		if (x != client->digging_data.x || y != client->digging_data.y || z != client->digging_data.z || client->digging_data.end.tv_sec == 0 || client->digging_data.item_id != item->id || client->digging_data.block_id != *block_id)
		{
			bedrock_log(LEVEL_DEBUG, "player digging: Mismatch in dig data - saved: X: %d Y: %d Z: %d T: %d I: %d B: %d - got: X: %d Y: %d Z: %d I: %d B: %d",
					client->digging_data.x, client->digging_data.y, client->digging_data.z, client->digging_data.end.tv_sec, client->digging_data.item_id, client->digging_data.block_id,
					x, y, z, item->id, *block_id);

			packet_send_block_change(client, x, y, z, *block_id, 0);
			return offset;
		}

		if (bedrock_time.tv_sec < client->digging_data.end.tv_sec || (bedrock_time.tv_sec == client->digging_data.end.tv_sec && bedrock_time.tv_nsec < client->digging_data.end.tv_nsec))
		{
			packet_send_block_change(client, x, y, z, *block_id, 0);
			return offset;
		}

		block = block_find_or_create(*block_id);
		*block_id = BLOCK_AIR;
		chunk->modified = true;
		column_set_pending(chunk->column, COLUMN_FLAG_DIRTY);

		// This is the height right *above* the highest block
		height = column_get_height_for(chunk->column, x, z);
		if (y == *height - 1)
		{
			uint8_t *height_block;

			do
				height_block = column_get_block(chunk->column, x, --(*height) - 1, z);
			while (*height && (height_block == NULL || *height_block == BLOCK_AIR));

			bedrock_log(LEVEL_DEBUG, "player digging: Adjusting height map of %d,%d to %d", x, z, *height);
		}

		// Notify players in render distance of the column to remove the block
		LIST_FOREACH(&chunk->column->players, node)
		{
			struct bedrock_client *c = node->data;

			packet_send_block_change(c, x, y, z, BLOCK_AIR, 0);
		}

		if (block->on_harvest != NULL && can_harvest(block, item))
			block->on_harvest(client, chunk, x, y, z, block);

		// If the chunk is all air delete it;
		for (i = 0; i < BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK; ++i)
			if (chunk->blocks[i] != BLOCK_AIR)
				break;
		if (i == BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK)
			chunk_free(chunk);
	}
	else if (status == DROP_ITEM)
	{
		/* Dropping currently held item */
		nbt_tag *tag = client_get_inventory_tag(client, client->selected_slot);
		if (tag != NULL)
		{
			uint16_t *id = nbt_read(tag, TAG_SHORT, 1, "id"), *data = nbt_read(tag, TAG_SHORT, 1, "Damage");
			uint8_t *count = nbt_read(tag, TAG_BYTE, 1, "Count");

			struct bedrock_dropped_item *di = bedrock_malloc(sizeof(struct bedrock_dropped_item));
			struct bedrock_column *col;

			di->item = item_find_or_create(*id);
			di->count = 1;
			di->data = *data;
			di->x = *client_get_pos_x(client);
			di->y = *client_get_pos_y(client);
			di->z = *client_get_pos_z(client);

			// XXX put in the direction the user is facing
			di->x += rand() % 4;
			di->z += rand() % 4;

			col = find_column_from_world_which_contains(client->world, di->x, di->z);
			if (col != NULL)
				column_add_item(client->column, di);
			else
				bedrock_free(di);

			bedrock_log(LEVEL_DEBUG, "player digging: %s drops a block of %s", client->name, item_find_or_create(*id)->name);

			--(*count);
			if (*count)
			{
				packet_send_set_slot(client, WINDOW_INVENTORY, client->selected_slot + 36, item_find_or_create(*id), *count, *data);
			}
			else
			{
				/* Item goes away */
				packet_send_set_slot(client, WINDOW_INVENTORY, client->selected_slot + 36, NULL, 0, 0);
				nbt_free(tag);
			}
		}
	}
	else
		bedrock_log(LEVEL_DEBUG, "player digging: Unrecognized dig status %d", status);

	return offset;
}
