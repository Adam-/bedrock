#include "server/bedrock.h"
#include "util/buffer.h"

struct bedrock_chunk
{
	/* Column this chunk is in */
	struct bedrock_column *column;
	/* Y coordinate */
	uint8_t y;

	/* A buffer of the compressed data use to fill below */
	bedrock_buffer *compressed_blocks, *compressed_data, *compressed_skylight, *compressed_blocklight;

	/* Only available if this chunk is decompressed! */
	bedrock_buffer *decompressed_blocks, *decompressed_data, *decompressed_skylight, *decompressed_blocklight;

	uint8_t *blocks;
	uint8_t *data;
	uint8_t *skylight;
	uint8_t *blocklight;
};

extern struct bedrock_chunk *chunk_create(struct bedrock_column *column, uint8_t y);
extern void chunk_free(struct bedrock_chunk *chunk);
