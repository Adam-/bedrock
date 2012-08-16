#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
#include "server/chunk.h"
#include "packet/packet_column_bulk.h"
#include "compression/compression.h"
#include "packet/packet_spawn_named_entity.h"
#include "packet/packet_spawn_dropped_item.h"

#define COLUMN_BULK_MAX 100
#define COLUMN_BUFFER_SIZE 8192

static void packet_column_bulk_send_players(struct bedrock_client *client, struct bedrock_column *column)
{
	bedrock_node *node;

	/* Tell this client about any clients in this column */
	LIST_FOREACH(&column->players, node)
	{
		struct bedrock_client *c = node->data;

		// Don't send a spawn for ourself
		if (client == c)
			continue;

		/* We only want players *in* this column not *near* this column */
		if (c->column == column)
		{
			/* Send this client */
			packet_send_spawn_named_entity(client, c);
			packet_send_spawn_named_entity(c, client);
		}
	}
}

static void packet_column_bulk_send_items(struct bedrock_client *client, struct bedrock_column *column)
{
	bedrock_node *node;

	/* Send any items in this column */
	LIST_FOREACH(&column->items, node)
	{
		struct bedrock_dropped_item *item = node->data;
		packet_send_spawn_dropped_item(client, item);
	}
}

void packet_column_bulk_add(struct bedrock_client *client, packet_column_bulk *columns, struct bedrock_column *column)
{
	bedrock_list_add(columns, column);

	if (columns->count >= COLUMN_BULK_MAX)
		packet_send_column_bulk(client, columns);
}

void packet_send_column_bulk(struct bedrock_client *client, packet_column_bulk *columns)
{
	compression_buffer *buffer;
	bedrock_packet packet;
	uint16_t count = columns->count;
	bedrock_node *node;
	int i;
	unsigned char fake_light[BEDROCK_DATA_LENGTH]; // Temporary until lighting is sorted
	uint32_t size;

	memset(fake_light, 0xFF, sizeof(fake_light));

	buffer = compression_compress_init(COLUMN_BUFFER_SIZE);
	bedrock_assert(buffer, return);

	packet_init(&packet, MAP_COLUMN_BULK);
	
	packet_pack_header(&packet, MAP_COLUMN_BULK);
	packet_pack_int(&packet, &count, sizeof(count));

	LIST_FOREACH(columns, node)
	{
		struct bedrock_column *column = node->data;

		for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct bedrock_chunk *chunk = column->chunks[i];

			if (!chunk)
				continue;

			compression_compress_deflate(buffer, chunk->blocks, BEDROCK_BLOCK_LENGTH);
		}

		for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct bedrock_chunk *chunk = column->chunks[i];

			if (!chunk)
				continue;

			compression_compress_deflate(buffer, chunk->data, BEDROCK_DATA_LENGTH);
		}


		for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct bedrock_chunk *chunk = column->chunks[i];

			if (!chunk)
				continue;

			compression_compress_deflate(buffer, chunk->blocklight, BEDROCK_DATA_LENGTH);
		}

		for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct bedrock_chunk *chunk = column->chunks[i];

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

	packet_pack(&packet, buffer->buffer->data, buffer->buffer->length);

	compression_compress_end(buffer);

	LIST_FOREACH(columns, node)
	{
		struct bedrock_column *column = node->data;
		uint16_t bitmask = 0;

		packet_pack_int(&packet, &column->x, sizeof(column->x));
		packet_pack_int(&packet, &column->z, sizeof(column->z));

		for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		{
			struct bedrock_chunk *chunk = column->chunks[i];

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
		struct bedrock_column *column = node->data;

		packet_column_bulk_send_players(client, column);
		packet_column_bulk_send_items(client, column);
	}

	bedrock_list_clear(columns);
}

