#include "server/column.h"
#include "util/util.h"
#include "compression/compression.h"

#define DATA_CHUNK_SIZE 1024

struct bedrock_chunk *chunk_create(struct bedrock_column *column, uint8_t y)
{
	struct bedrock_chunk *chunk = bedrock_malloc(sizeof(struct bedrock_chunk));
	chunk->column = column;
	chunk->y = y;
	return chunk;
}

void chunk_free(struct bedrock_chunk *chunk)
{
	if (!chunk)
		return;

	chunk_compress(chunk);

	bedrock_buffer_free(chunk->compressed_blocks);
	bedrock_buffer_free(chunk->compressed_data2);
	bedrock_buffer_free(chunk->compressed_skylight);
	bedrock_buffer_free(chunk->compressed_blocklight);

	bedrock_free(chunk);
}

void chunk_decompress(struct bedrock_chunk *chunk)
{
	compression_buffer *buffer;

	bedrock_assert(chunk != NULL, return);
	bedrock_assert(!chunk->data == !chunk->skylight && !chunk->data == !chunk->blocklight, return);

	if (chunk->decompressed_data2 != NULL)
		return;

	buffer = compression_decompress(DATA_CHUNK_SIZE, chunk->compressed_blocks->data, chunk->compressed_blocks->length);
	chunk->decompressed_blocks = buffer->buffer;
	buffer->buffer = NULL;
	compression_decompress_end(buffer);

	buffer = compression_decompress(DATA_CHUNK_SIZE, chunk->compressed_data2->data, chunk->compressed_data2->length);
	chunk->decompressed_data2 = buffer->buffer;
	buffer->buffer = NULL;
	compression_decompress_end(buffer);

	buffer = compression_decompress(DATA_CHUNK_SIZE, chunk->compressed_skylight->data, chunk->compressed_skylight->length);
	chunk->decompressed_skylight = buffer->buffer;
	buffer->buffer = NULL;
	compression_decompress_end(buffer);

	buffer = compression_decompress(DATA_CHUNK_SIZE, chunk->compressed_blocklight->data, chunk->compressed_blocklight->length);
	chunk->decompressed_blocklight = buffer->buffer;
	buffer->buffer = NULL;
	compression_decompress_end(buffer);

	chunk->blocks = chunk->decompressed_blocks->data;
	chunk->data = chunk->decompressed_data2->data;
	chunk->skylight = chunk->decompressed_skylight->data;
	chunk->blocklight = chunk->decompressed_blocklight->data;
}

void chunk_compress(struct bedrock_chunk *chunk)
{
	chunk->blocks = NULL;
	chunk->data = NULL;
	chunk->skylight = NULL;
	chunk->blocklight = NULL;

	bedrock_buffer_free(chunk->decompressed_blocks);
	chunk->decompressed_blocks = NULL;

	bedrock_buffer_free(chunk->decompressed_data2);
	chunk->decompressed_data2 = NULL;

	bedrock_buffer_free(chunk->decompressed_skylight);
	chunk->decompressed_skylight = NULL;

	bedrock_buffer_free(chunk->decompressed_blocklight);
	chunk->decompressed_blocklight = NULL;
}
