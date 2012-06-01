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

	bedrock_buffer_free(chunk->compressed_data);

	bedrock_free(chunk);
}

void chunk_decompress(struct bedrock_chunk *chunk)
{
	compression_buffer *buffer;

	bedrock_assert(chunk != NULL, return);
	bedrock_assert(!chunk->data == !chunk->skylight && !chunk->data == !chunk->blocklight, return);

	if (chunk->decompressed_data != NULL)
		return;

	buffer = compression_decompress(DATA_CHUNK_SIZE, chunk->compressed_data->data, chunk->compressed_data->length);
	chunk->decompressed_data = buffer->buffer;
	buffer->buffer = NULL;
	compression_decompress_end(buffer);

	chunk->blocks = chunk->decompressed_data->data;
	chunk->data = chunk->decompressed_data->data + 2048; //
	chunk->skylight = chunk->decompressed_data->data + 4096; // XXX
	chunk->blocklight = chunk->decompressed_data->data + 6144; // XXX
}

void chunk_compress(struct bedrock_chunk *chunk)
{
	chunk->blocks = NULL;
	chunk->data = NULL;
	chunk->skylight = NULL;
	chunk->blocklight = NULL;

	bedrock_buffer_free(chunk->decompressed_data);
	chunk->decompressed_data = NULL;
}
