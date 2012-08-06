#include "server/client.h"
#include "server/packet.h"
#include "server/column.h"
#include "compression/compression.h"
#include "nbt/nbt.h"
#include "packet/packet_destroy_entity.h"
#include "packet/packet_spawn_dropped_item.h"
#include "packet/packet_spawn_named_entity.h"

#define COLUMN_BUFFER_SIZE 8192

static void packet_column_send_players(struct bedrock_client *client, struct bedrock_column *column)
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

static void packet_column_send_items(struct bedrock_client *client, struct bedrock_column *column)
{
	bedrock_node *node;

	/* Send any items in this column */
	LIST_FOREACH(&column->items, node)
	{
		struct bedrock_dropped_item *item = node->data;
		packet_send_spawn_dropped_item(client, item);
	}
}

static void packet_column_remove_players(struct bedrock_client *client, struct bedrock_column *column)
{
	bedrock_node *node;

	/* This column is going away, find players in this column */
	LIST_FOREACH(&column->players, node)
	{
		struct bedrock_client *cl = node->data;

		/* We only want players *in* this column not *near* this column */
		if (cl->column == column)
		{
			/* Remove these clients from each other */
			packet_send_destroy_entity_player(client, cl);
			packet_send_destroy_entity_player(cl, client);
		}
	}
}

static void packet_column_remove_items(struct bedrock_client *client, struct bedrock_column *column)
{
	bedrock_node *node;

	/* Remove items in this column */
	LIST_FOREACH(&column->items, node)
	{
		struct bedrock_dropped_item *item = node->data;
		packet_send_destroy_entity_dropped_item(client, item);
	}
}

void packet_send_column(struct bedrock_client *client, struct bedrock_column *column)
{
	bedrock_packet packet;
	uint8_t b;
	uint16_t bitmask;
	compression_buffer *buffer;
	uint32_t i;

	buffer = compression_compress_init(COLUMN_BUFFER_SIZE);
	bedrock_assert(buffer, return);

	packet_init(&packet, MAP_COLUMN);

	packet_pack_header(&packet, MAP_COLUMN);
	packet_pack_int(&packet, &column->x, sizeof(column->x));
	packet_pack_int(&packet, &column->z, sizeof(column->z));
	b = 1;
	packet_pack_int(&packet, &b, sizeof(b)); // Ground up continuous

	bitmask = 0;
	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		chunk_decompress(chunk);

		bitmask |= 1 << chunk->y;

		compression_compress_deflate(buffer, chunk->blocks, BEDROCK_BLOCK_LENGTH);
	}
	packet_pack_int(&packet, &bitmask, sizeof(bitmask)); // primary bit map

	bitmask = 0;
	packet_pack_int(&packet, &bitmask, sizeof(bitmask)); // add bit map

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
		unsigned char fake_light[BEDROCK_DATA_LENGTH]; // Temporary until lighting is sorted
		memset(fake_light, 0xFF, sizeof(fake_light));

		if (!chunk)
			continue;

		compression_compress_deflate(buffer, fake_light, BEDROCK_DATA_LENGTH);
		//compression_compress_deflate(buffer, chunk->skylight, BEDROCK_DATA_LENGTH);
	}

	{
		compression_buffer *biomes = compression_decompress(BEDROCK_BUFFER_DEFAULT_SIZE, column->biomes->data, column->biomes->length);
		bedrock_assert(biomes->buffer->length == BEDROCK_BIOME_LENGTH, ;);
		compression_compress_deflate_finish(buffer, biomes->buffer->data, biomes->buffer->length);
		compression_decompress_end(biomes);
	}

	i = buffer->buffer->length;
	packet_pack_int(&packet, &i, sizeof(i)); // length

	packet_pack(&packet, buffer->buffer->data, buffer->buffer->length);
	
	compression_compress_end(buffer);

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		if (column->chunks[i])
			chunk_compress(column->chunks[i]);
	
	client_send_packet(client, &packet);

	packet_column_send_players(client, column);
	packet_column_send_items(client, column);
}

void packet_send_column_empty(struct bedrock_client *client, struct bedrock_column *column)
{
	bedrock_packet packet;
	uint8_t b;
	uint16_t s;
	uint32_t i;

	packet_column_remove_players(client, column);
	packet_column_remove_items(client, column);

	packet_init(&packet, SPAWN_DROPPED_ITEM);

	packet_pack_header(&packet, MAP_COLUMN);
	packet_pack_int(&packet, nbt_read(column->data, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	packet_pack_int(&packet, nbt_read(column->data, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = 1;
	packet_pack_int(&packet, &b, sizeof(b)); // Ground up continuous

	s = 0;
	packet_pack_int(&packet, &s, sizeof(s)); // Primary bitmap
	packet_pack_int(&packet, &s, sizeof(s)); // Add bitmap

	i = 0;
	packet_pack_int(&packet, &i, sizeof(i)); // Size

	client_send_packet(client, &packet);
}
