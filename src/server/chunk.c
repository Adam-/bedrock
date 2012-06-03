#include "server/column.h"
#include "util/util.h"
#include "compression/compression.h"
#include "util/memory.h"
#include "server/config.h"
#include "nbt/nbt.h"

#define BLOCK_CHUNK_SIZE 8192
#define DATA_CHUNK_SIZE 16384

bedrock_memory_pool chunk_pool = BEDROCK_MEMORY_POOL_INIT;

struct bedrock_chunk *chunk_create(struct bedrock_column *column, uint8_t y, nbt_tag *chunk_tag)
{
	struct bedrock_chunk *chunk;
	compression_buffer *buffer;
	struct nbt_tag_byte_array *byte_array;

	chunk = bedrock_malloc_pool(&chunk_pool, sizeof(struct bedrock_chunk));
	chunk->column = column;
	chunk->y = y;

	buffer = compression_compress_init(&chunk_pool, BLOCK_CHUNK_SIZE);

	byte_array = &nbt_get(chunk_tag, TAG_BYTE_ARRAY, 1, "Blocks")->payload.tag_byte_array;
	bedrock_assert(byte_array->length == BEDROCK_BLOCK_LENGTH, ;);
	compression_compress_deflate(buffer, (const unsigned char *) byte_array->data, byte_array->length);

	byte_array = &nbt_get(chunk_tag, TAG_BYTE_ARRAY, 1, "Data")->payload.tag_byte_array;
	bedrock_assert(byte_array->length == BEDROCK_DATA_LENGTH, ;);
	compression_compress_deflate(buffer, (const unsigned char *) byte_array->data, byte_array->length);

	byte_array = &nbt_get(chunk_tag, TAG_BYTE_ARRAY, 1, "SkyLight")->payload.tag_byte_array;
	bedrock_assert(byte_array->length == BEDROCK_DATA_LENGTH, ;);
	compression_compress_deflate(buffer, (const unsigned char *) byte_array->data, byte_array->length);

	byte_array = &nbt_get(chunk_tag, TAG_BYTE_ARRAY, 1, "BlockLight")->payload.tag_byte_array;
	bedrock_assert(byte_array->length == BEDROCK_DATA_LENGTH, ;);
	compression_compress_deflate_finish(buffer, (const unsigned char *) byte_array->data, byte_array->length);

	bedrock_buffer_resize(buffer->buffer, buffer->buffer->length);
	chunk->compressed_data = buffer->buffer;
	buffer->buffer = NULL;

	compression_compress_end(buffer);

	return chunk;
}

void chunk_free(struct bedrock_chunk *chunk)
{
	if (!chunk)
		return;

	chunk_compress(chunk);

	bedrock_buffer_free(chunk->compressed_data);

	bedrock_free_pool(&chunk_pool, chunk);
}

void chunk_decompress(struct bedrock_chunk *chunk)
{
	compression_buffer *buffer;

	bedrock_assert(chunk != NULL, return);
	bedrock_assert(!chunk->data == !chunk->skylight && !chunk->data == !chunk->blocklight, return);

	if (chunk->decompressed_data != NULL)
		return;

	buffer = compression_decompress(&chunk_pool, DATA_CHUNK_SIZE, chunk->compressed_data->data, chunk->compressed_data->length);
	chunk->decompressed_data = buffer->buffer;
	buffer->buffer = NULL;
	compression_decompress_end(buffer);

	bedrock_assert(chunk->decompressed_data->length == BEDROCK_BLOCK_LENGTH + BEDROCK_DATA_LENGTH + BEDROCK_DATA_LENGTH + BEDROCK_DATA_LENGTH, ;);

	chunk->blocks = chunk->decompressed_data->data;
	chunk->data = chunk->blocks + BEDROCK_BLOCK_LENGTH;
	chunk->skylight = chunk->data + BEDROCK_DATA_LENGTH;
	chunk->blocklight = chunk->skylight + BEDROCK_DATA_LENGTH;
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
