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
	bedrock_node *node;
	compression_buffer *buffer;
	const struct nbt_tag_byte_array *blocks;
	int i;

	client_send_header(client, MAP_COLUMN);
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = 1;
	client_send_int(client, &b, sizeof(b)); // Ground up continuous

	bitmask = 0;
	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		chunk_decompress(chunk);
		bitmask |= 1 << chunk->y;
	}
	client_send_int(client, &bitmask, sizeof(bitmask)); // primary bit map

	bitmask = 0;
	client_send_int(client, &bitmask, sizeof(bitmask)); // add bit map

	buffer = compression_compress_init(COLUMN_BUFFER_SIZE);
	bedrock_assert(buffer, return);

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		bedrock_assert(sizeof(chunk->blocks) == 4096, goto error);

		compression_compress_deflate(buffer, chunk->blocks, sizeof(chunk->blocks));
	}

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		compression_compress_deflate(buffer, chunk->data, 2048);
	}

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		compression_compress_deflate(buffer, chunk->blocklight, 2048);
	}

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		compression_compress_deflate(buffer, chunk->skylight, 2048);
	}

	blocks = nbt_read(column->data, TAG_BYTE_ARRAY, 2, "Level", "Biomes");
	bedrock_assert(blocks->length == 256, goto error);

	compression_compress_deflate(buffer, blocks->data, blocks->length);

	uint32_t ii = buffer->buffer->length;
	client_send_int(client, &ii, sizeof(ii)); // length
	ii = 0;
	client_send_int(client, &ii, sizeof(ii)); // not used

	client_send(client, buffer->buffer->data, buffer->buffer->length);

 error:
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
