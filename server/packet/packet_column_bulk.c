#include "server/client.h"
#include "server/packet.h"
#include "server/packets.h"
#include "server/column.h"
#include "server/chunk.h"
#include "util/compression.h"

#define COLUMN_BULK_MAX 100
#define COLUMN_BUFFER_SIZE 8192

static void packet_column_bulk_send_players(struct client *client, struct column *column)
{
	bedrock_node *node;

	/* Tell this client about any clients in this column */
	LIST_FOREACH(&column->players, node)
	{
		struct client *c = node->data;

		// Don't send a spawn for ourself
		if (client == c)
			continue;

		/* We only want players *in* this column not *near* this column */
		if (c->column == column)
		{
			/* Send this client */
			packet_send_spawn_player(client, c);
		}
	}
}

static void packet_column_bulk_send_items(struct client *client, struct column *column)
{
	bedrock_node *node;

	/* Send any items in this column */
	LIST_FOREACH(&column->items, node)
	{
		struct dropped_item *item = node->data;
		packet_spawn_object_item(client, item);
	}
}

void packet_column_bulk_add(struct client *client, packet_column_bulk *columns, struct column *column)
{
	bedrock_list_add(columns, column);

	if (columns->count >= COLUMN_BULK_MAX)
		packet_send_column_bulk(client, columns);
}

void packet_send_column_bulk(struct client *client, packet_column_bulk *columns)
{
	bedrock_packet packet;
	bedrock_buffer *buffer;
	bedrock_node *node;
	unsigned char fake_light[BEDROCK_DATA_LENGTH]; // Temporary until lighting is sorted

	memset(fake_light, 0xFF, sizeof(fake_light));

	packet_init(&packet, SERVER_MAP_COLUMN_BULK);
	
	packet_pack_bool(&packet, true); // Skylight
	packet_pack_varint(&packet, columns->count);

	LIST_FOREACH(columns, node)
	{
		struct column *column = node->data;
		uint16_t bitmask = 0;

		packet_pack_int(&packet, column->x);
		packet_pack_int(&packet, column->z);

		buffer = bedrock_buffer_create("column buffer", NULL, 0, 8192);

		for (int i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct chunk *chunk = column->chunks[i];
			uint16_t blocks[BEDROCK_BLOCK_LENGTH];

			if (!chunk)
				continue;

			bitmask |= 1 << chunk->y;

			for (int j = 0; j < BEDROCK_BLOCK_LENGTH; ++j)
			{
				uint8_t block = chunk->blocks[j];
				uint8_t data = chunk->data[j / 2];
				if ((j % 2) == 0)
					data >>= 4;
				else
					data &= 0xF;
				blocks[j] = (block << 4) | data;
			}

			bedrock_buffer_append(buffer, blocks, sizeof(blocks));
		}

		for (int i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct chunk *chunk = column->chunks[i];

			if (!chunk)
				continue;

			bedrock_buffer_append(buffer, fake_light, BEDROCK_DATA_LENGTH);
		}

		for (int i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct chunk *chunk = column->chunks[i];

			if (!chunk)
				continue;

			bedrock_buffer_append(buffer, fake_light, BEDROCK_DATA_LENGTH);
		}

		bedrock_buffer_append(buffer, column->biomes, BEDROCK_BIOME_LENGTH);

		packet_pack_short(&packet, bitmask);

		packet_pack(&packet, buffer->data, buffer->length);

		bedrock_buffer_free(buffer);
	}

	client_send_packet(client, &packet);

	LIST_FOREACH(columns, node)
	{
		struct column *column = node->data;

		packet_column_bulk_send_players(client, column);
		packet_column_bulk_send_items(client, column);
	}

	bedrock_list_clear(columns);
}

