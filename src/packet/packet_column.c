#include "server/client.h"
#include "packet/packet.h"
#include "server/column.h"
#include "compression/compression.h"
#include "nbt/nbt.h"

#define COLUMN_BUFFER_SIZE 65536

void packet_send_column(struct bedrock_client *client, struct bedrock_column *column)
{
	uint8_t b;
	uint16_t bitmask;
	uint32_t i;
	bedrock_buffer *buffer;

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

		bitmask |= 1 << chunk->y;
	}
	client_send_int(client, &bitmask, sizeof(bitmask)); // primary bit map

	bitmask = 0;
	client_send_int(client, &bitmask, sizeof(bitmask)); // add bit map

	buffer = bedrock_buffer_create(NULL, 0, COLUMN_BUFFER_SIZE);
	bedrock_assert(buffer != NULL, return);

	bool first = false;
	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		if (!first)
		{
			first = true;
			bedrock_buffer_append(buffer, chunk->compressed_blocks->data, chunk->compressed_blocks->length);
		}
		else
			bedrock_buffer_append(buffer, chunk->compressed_blocks->data + ZLIB_HEADER_SIZE, chunk->compressed_blocks->length - ZLIB_HEADER_SIZE);
	}

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		bedrock_buffer_append(buffer, chunk->compressed_data->data + ZLIB_HEADER_SIZE, chunk->compressed_data->length - ZLIB_HEADER_SIZE);
	}

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		bedrock_buffer_append(buffer, chunk->compressed_blocklight->data + ZLIB_HEADER_SIZE, chunk->compressed_blocklight->length - ZLIB_HEADER_SIZE);
	}

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		bedrock_buffer_append(buffer, chunk->compressed_skylight->data + ZLIB_HEADER_SIZE, chunk->compressed_skylight->length - ZLIB_HEADER_SIZE);
	}

	bedrock_buffer_append(buffer, column->compressed_biomes->data + ZLIB_HEADER_SIZE, column->compressed_biomes->length - ZLIB_HEADER_SIZE);

	i = buffer->length;
	client_send_int(client, &i, sizeof(i)); // length
	i = 0;
	client_send_int(client, &i, sizeof(i)); // not used

	client_send(client, buffer->data, buffer->length);

	bedrock_buffer_free(buffer);
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
