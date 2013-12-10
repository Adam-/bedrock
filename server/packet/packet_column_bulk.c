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
			packet_send_spawn_player(c, client);
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
	compression_buffer *buffer;
	bedrock_packet packet;
	uint16_t count = columns->count;
	bedrock_node *node;
	int i;
	unsigned char fake_light[BEDROCK_DATA_LENGTH]; // Temporary until lighting is sorted
	uint32_t size;
	uint8_t b;

	memset(fake_light, 0xFF, sizeof(fake_light));

	buffer = compression_compress_init(COLUMN_BUFFER_SIZE);
	bedrock_assert(buffer, return);

	packet_init(&packet, SERVER_MAP_COLUMN_BULK);
	
	packet_pack_int(&packet, &count, sizeof(count));

	LIST_FOREACH(columns, node)
	{
		struct column *column = node->data;

		for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct chunk *chunk = column->chunks[i];

			if (!chunk)
				continue;

			compression_compress_deflate(buffer, chunk->blocks, BEDROCK_BLOCK_LENGTH);
		}

		for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct chunk *chunk = column->chunks[i];

			if (!chunk)
				continue;

			compression_compress_deflate(buffer, chunk->data, BEDROCK_DATA_LENGTH);
		}

		for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct chunk *chunk = column->chunks[i];

			if (!chunk)
				continue;

			compression_compress_deflate(buffer, chunk->blocklight, BEDROCK_DATA_LENGTH);
		}

		for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct chunk *chunk = column->chunks[i];

			if (!chunk)
				continue;

			//compression_compress_deflate(buffer, chunk->skylight, BEDROCK_DATA_LENGTH);
			compression_compress_deflate(buffer, fake_light, BEDROCK_DATA_LENGTH);
		}

		if (node->next)
			compression_compress_deflate(buffer, column->biomes, BEDROCK_BIOME_LENGTH);
		else
			compression_compress_deflate_finish(buffer, column->biomes, BEDROCK_BIOME_LENGTH);
	}

	size = buffer->buffer->length;
	packet_pack_int(&packet, &size, sizeof(size));

	b = 1; // Skylight
	packet_pack_int(&packet, &b, sizeof(b));

	packet_pack(&packet, buffer->buffer->data, buffer->buffer->length);

	compression_compress_end(buffer);

	LIST_FOREACH(columns, node)
	{
		struct column *column = node->data;
		uint16_t bitmask = 0;

		packet_pack_int(&packet, &column->x, sizeof(column->x));
		packet_pack_int(&packet, &column->z, sizeof(column->z));

		for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct chunk *chunk = column->chunks[i];

			if (!chunk)
				continue;

			bitmask |= 1 << chunk->y;
		}
		packet_pack_int(&packet, &bitmask, sizeof(bitmask));

		bitmask = 0;
		packet_pack_int(&packet, &bitmask, sizeof(bitmask));
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

