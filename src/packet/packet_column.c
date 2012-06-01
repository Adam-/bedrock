#include "server/client.h"
#include "packet/packet.h"
#include "server/column.h"
#include "compression/compression.h"
#include "nbt/nbt.h"

#define COLUMN_BUFFER_SIZE 8192

void packet_send_column(struct bedrock_client *client, struct bedrock_column *column)
{
	uint8_t b;
	uint16_t bitmask;
	compression_buffer *buffer;
	uint32_t i;

	client_send_header(client, MAP_COLUMN);
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = 1;
	client_send_int(client, &b, sizeof(b)); // Ground up continuous

	buffer = compression_compress_init(COLUMN_BUFFER_SIZE);
	bedrock_assert(buffer, return);

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
	client_send_int(client, &bitmask, sizeof(bitmask)); // primary bit map

	bitmask = 0;
	client_send_int(client, &bitmask, sizeof(bitmask)); // add bit map

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

		compression_compress_deflate(buffer, chunk->skylight, BEDROCK_DATA_LENGTH);
	}

	{
		compression_buffer *biomes = compression_decompress(BEDROCK_BUFFER_DEFAULT_SIZE, column->biomes->data, column->biomes->length);
		bedrock_assert(biomes->buffer->length == BEDROCK_BIOME_LENGTH, ;);
		compression_compress_deflate_finish(buffer, (const unsigned char *) biomes->buffer->data, biomes->buffer->length);
		compression_decompress_end(biomes);
	}

	i = buffer->buffer->length;
	client_send_int(client, &i, sizeof(i)); // length
	i = 0;
	client_send_int(client, &i, sizeof(i)); // not used

	client_send(client, buffer->buffer->data, buffer->buffer->length);

	compression_compress_end(buffer);

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		if (column->chunks[i])
			chunk_compress(column->chunks[i]);
}

void packet_send_column_empty(struct bedrock_client *client, struct bedrock_column *column)
{
	uint8_t b;
	uint16_t s;
	uint32_t i;

	client_send_header(client, MAP_COLUMN);
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = 1;
	client_send_int(client, &b, sizeof(b)); // Ground up continuous

	s = 0;
	client_send_int(client, &s, sizeof(s)); // Primary bitmap
	client_send_int(client, &s, sizeof(s)); // Add bitmap

	i = 0;
	client_send_int(client, &i, sizeof(i)); // Size
	client_send_int(client, &i, sizeof(i)); // Unused
}
