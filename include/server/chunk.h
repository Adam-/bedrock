#ifndef BEDROCK_SERVER_CHUNK_H
#define BEDROCK_SERVER_CHUNK_H

#include "server/bedrock.h"
#include "util/buffer.h"

struct bedrock_chunk
{
	/* Column this chunk is in */
	struct bedrock_column *column;
	/* Y coordinate */
	uint8_t y;
	bool modified;

	/* A buffer of the compressed data use to fill below */
	bedrock_buffer *compressed_data;

	/* Only available if this chunk is decompressed! */
	bedrock_buffer *decompressed_data;
	uint8_t *blocks;
	uint8_t *data;
	uint8_t *skylight;
	uint8_t *blocklight;
};

extern struct bedrock_memory_pool chunk_pool;

extern struct bedrock_chunk *chunk_create(struct bedrock_column *column, uint8_t y);
extern struct bedrock_chunk *chunk_load(struct bedrock_column *column, uint8_t y, nbt_tag *tag);
extern uint8_t *chunk_get_block(struct bedrock_chunk *chunk, int32_t x, uint8_t y, int32_t z);
extern void chunk_free(struct bedrock_chunk *chunk);
extern void chunk_decompress(struct bedrock_chunk *chunk);
extern void chunk_compress(struct bedrock_chunk *chunk);
extern struct bedrock_chunk *find_chunk_which_contains(struct bedrock_world *world, int32_t x, uint8_t y, int32_t z);

#endif // BEDROCK_SERVER_CHUNK_H
