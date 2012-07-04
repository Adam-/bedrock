#include "server/column.h"
#include "util/util.h"
#include "compression/compression.h"
#include "util/memory.h"
#include "config/hard.h"
#include "nbt/nbt.h"

#define BLOCK_CHUNK_SIZE 8192
#define DATA_CHUNK_SIZE 16384

struct bedrock_memory_pool chunk_pool = BEDROCK_MEMORY_POOL_INIT("chunk memory pool");

struct bedrock_chunk *chunk_create(struct bedrock_column *column, uint8_t y)
{
	struct bedrock_chunk *chunk;
	compression_buffer *buffer;
	unsigned char empty_chunk[BEDROCK_BLOCK_LENGTH + BEDROCK_DATA_LENGTH + BEDROCK_DATA_LENGTH + BEDROCK_DATA_LENGTH];

	bedrock_assert(y < BEDROCK_CHUNKS_PER_COLUMN, return NULL);
	bedrock_assert(column != NULL && column->chunks[y] == NULL, return NULL);

	chunk = bedrock_malloc_pool(&chunk_pool, sizeof(struct bedrock_chunk));
	chunk->column = column;
	chunk->y = y;

	buffer = compression_compress_init(&chunk_pool, BLOCK_CHUNK_SIZE);

	memset(&empty_chunk, 0, sizeof(empty_chunk));
	compression_compress_deflate(buffer, empty_chunk, sizeof(empty_chunk));

	bedrock_buffer_resize(buffer->buffer, buffer->buffer->length);
	chunk->compressed_data = buffer->buffer;
	buffer->buffer = NULL;

	compression_compress_end(buffer);

	column->chunks[y] = chunk;

	return chunk;
}

struct bedrock_chunk *chunk_load(struct bedrock_column *column, uint8_t y, nbt_tag *chunk_tag)
{
	struct bedrock_chunk *chunk;
	compression_buffer *buffer;
	struct nbt_tag_byte_array *byte_array;

	bedrock_assert(y < BEDROCK_CHUNKS_PER_COLUMN, return NULL);
	bedrock_assert(column != NULL && column->chunks[y] == NULL, return NULL);

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

	column->chunks[y] = chunk;

	return chunk;
}

uint8_t *chunk_get_block(struct bedrock_chunk *chunk, int32_t x, uint8_t y, int32_t z)
{
	uint16_t block_index;

	bedrock_assert(chunk->blocks != NULL, return NULL);

	x %= BEDROCK_BLOCKS_PER_CHUNK;
	y %= BEDROCK_BLOCKS_PER_CHUNK;
	z %= BEDROCK_BLOCKS_PER_CHUNK;

	if (x < 0)
		x = BEDROCK_BLOCKS_PER_CHUNK - abs(x);
	if (z < 0)
		z = BEDROCK_BLOCKS_PER_CHUNK - abs(z);

	block_index = (y * BEDROCK_BLOCKS_PER_CHUNK + z) * BEDROCK_BLOCKS_PER_CHUNK + x;

	bedrock_assert(block_index < BEDROCK_BLOCK_LENGTH, return NULL);

	return &chunk->blocks[block_index];
}

void chunk_free(struct bedrock_chunk *chunk)
{
	int i;

	if (!chunk)
		return;

	bedrock_log(LEVEL_DEBUG, "chunk: Freeing chunk %d in column %d,%d", chunk->y, chunk->column->x, chunk->column->z);

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		if (chunk->column->chunks[i] == chunk)
			chunk->column->chunks[i] = NULL;

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
	bedrock_buffer_resize(buffer->buffer, buffer->buffer->length);
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
	if (chunk->decompressed_data == NULL)
		return;

	if (chunk->modified)
	{
		compression_buffer *buffer = compression_compress_init(&chunk_pool, BLOCK_CHUNK_SIZE);

		compression_compress_deflate(buffer, chunk->blocks, BEDROCK_BLOCK_LENGTH);
		compression_compress_deflate(buffer, chunk->data, BEDROCK_DATA_LENGTH);
		compression_compress_deflate(buffer, chunk->skylight, BEDROCK_DATA_LENGTH);
		compression_compress_deflate_finish(buffer, chunk->blocklight, BEDROCK_DATA_LENGTH);

		bedrock_buffer_free(chunk->compressed_data);

		chunk->compressed_data = buffer->buffer;
		bedrock_buffer_resize(chunk->compressed_data, chunk->compressed_data->length);

		buffer->buffer = NULL;
		compression_compress_end(buffer);
	}

	chunk->blocks = NULL;
	chunk->data = NULL;
	chunk->skylight = NULL;
	chunk->blocklight = NULL;

	bedrock_buffer_free(chunk->decompressed_data);
	chunk->decompressed_data = NULL;
}

struct bedrock_chunk *find_chunk_which_contains(struct bedrock_world *world, int32_t x, uint8_t y, int32_t z)
{
	struct bedrock_region *region;
	struct bedrock_column *column;

	region = find_region_which_contains(world, x, z);
	if (region == NULL)
		return NULL;

	column = find_column_which_contains(region, x, z);
	if (column == NULL)
		return NULL;

	bedrock_assert(y / BEDROCK_BLOCKS_PER_CHUNK < BEDROCK_CHUNKS_PER_COLUMN, return NULL);
	return column->chunks[y / BEDROCK_BLOCKS_PER_CHUNK];
}
