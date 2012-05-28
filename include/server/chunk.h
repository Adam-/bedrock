#include "server/bedrock.h"
#include "util/buffer.h"

struct bedrock_chunk
{
	/* Column this chunk is in */
	struct bedrock_column *column;
	/* Y coordinate */
	uint8_t y;

	/* Actual blocks */
	uint8_t blocks[BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK * BEDROCK_BLOCKS_PER_CHUNK];

	/* A buffer of the compressed data use to fill below */
	bedrock_buffer *compressed_data;

	/* Only available if this chunk is decompressed! */
	bedrock_buffer *decompressed_data;
	uint8_t *data;
	uint8_t *skylight;
	uint8_t *blocklight;
};

extern struct bedrock_chunk *chunk_create(struct bedrock_column *column, uint8_t y);
extern void chunk_free(struct bedrock_chunk *chunk);
