#ifndef BEDROCK_SERVER_CHUNK_H
#define BEDROCK_SERVER_CHUNK_H

#include "server/bedrock.h"
#include "util/buffer.h"

struct chunk
{
	/* Column this chunk is in */
	struct column *column;
	/* Y coordinate */
	uint8_t y;

	/* These are pointers into the NBT structure for the column */
	uint8_t *blocks;
	uint8_t *data;
	uint8_t *skylight;
	uint8_t *blocklight;
};

extern struct chunk *chunk_create(struct column *column, uint8_t y);
extern struct chunk *chunk_load(struct column *column, uint8_t y, nbt_tag *tag);
extern uint8_t *chunk_get_block(struct chunk *chunk, int32_t x, uint8_t y, int32_t z);
extern uint8_t *chunk_get_block_from_world(struct world *world, int32_t x, uint8_t y, int32_t z);
extern uint8_t chunk_get_data(struct chunk *chunk, int32_t x, uint8_t y, int32_t z);
extern void chunk_free(struct chunk *chunk);
extern struct chunk *find_chunk_which_contains(struct world *world, int32_t x, uint8_t y, int32_t z);
extern struct chunk *find_chunk_from_column_which_contains(struct column *column, int32_t x, uint8_t y, int32_t z);

#endif // BEDROCK_SERVER_CHUNK_H
