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
	//compression_buffer *buffer;
	const struct nbt_tag_byte_array *blocks;
	int i;

	client_send_header(client, MAP_COLUMN);
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = 0;
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

	//buffer = compression_compress_init_type(COLUMN_BUFFER_SIZE, Z_NO_COMPRESSION);
	//bedrock_assert(buffer, return);

	bedrock_buffer *buffer2 = bedrock_buffer_create(NULL, 0, 1024);

	bool first = false;
	struct bedrock_chunk *firstb = NULL;
	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		if (!first)
		{
			// ??????????????????????
			first = true;
			firstb = chunk;

			bedrock_buffer_append(buffer2, chunk->compressed_blocks->data, chunk->compressed_blocks->length);
		}
		else
			bedrock_buffer_append(buffer2, chunk->compressed_blocks->data + ZLIB_HEADER_SIZE, chunk->compressed_blocks->length - ZLIB_HEADER_SIZE);
	}

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		bedrock_buffer_append(buffer2, chunk->compressed_data2->data + ZLIB_HEADER_SIZE, chunk->compressed_data2->length - ZLIB_HEADER_SIZE);
	}

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		bedrock_buffer_append(buffer2, chunk->compressed_blocklight->data + ZLIB_HEADER_SIZE, chunk->compressed_blocklight->length - ZLIB_HEADER_SIZE);
	}

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
	{
		struct bedrock_chunk *chunk = column->chunks[i];

		if (!chunk)
			continue;

		bedrock_buffer_append(buffer2, chunk->compressed_skylight->data + ZLIB_HEADER_SIZE, chunk->compressed_skylight->length - ZLIB_HEADER_SIZE);
	}

	/*blocks = nbt_read(column->data, TAG_BYTE_ARRAY, 2, "Level", "Biomes");
	bedrock_assert(blocks->length == 256, goto error);

	compression_compress_deflate(buffer, blocks->data, blocks->length);*/

/*	assert(first);
	assert(firstb);
	compression_buffer *reallybig = compression_decompress(4096, buffer->buffer->data + ZLIB_HEADER_SIZE, buffer->buffer->length - ZLIB_HEADER_SIZE);
	assert(memcmp(reallybig->buffer->data + 5, firstb->data, firstb->length) == 0);
	exit(-1);
	assert(0);*/

	assert(firstb);
	chunk_decompress(firstb);
	compression_buffer *reallybig = compression_decompress(4096, buffer2->data, buffer2->length);
	assert(memcmp(firstb->blocks, reallybig->buffer->data, 4096) == 0);

	//uint32_t ii = buffer->buffer->length - ZLIB_HEADER_SIZE;
	uint32_t ii = buffer2->length;
	client_send_int(client, &ii, sizeof(ii)); // length
	ii = 0;
	client_send_int(client, &ii, sizeof(ii)); // not used

	///client_send(client, buffer->buffer->data + ZLIB_HEADER_SIZE, buffer->buffer->length - ZLIB_HEADER_SIZE);
	client_send(client, buffer2->data, buffer2->length);

 //error:
//	compression_compress_end(buffer);
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
