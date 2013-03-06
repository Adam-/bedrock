#include "server/column.h"
#include "util/compression.h"
#include "util/util.h"
#include "util/memory.h"
#include "config/hard.h"
#include "nbt/nbt.h"

#define BLOCK_CHUNK_SIZE 8192
#define DATA_CHUNK_SIZE 16384

struct chunk *chunk_create(struct column *column, uint8_t y)
{
	struct chunk *chunk;
	nbt_tag *sections, *compound, *tag;

	bedrock_assert(y < BEDROCK_CHUNKS_PER_COLUMN, return NULL);
	bedrock_assert(column != NULL && column->chunks[y] == NULL, return NULL);

	chunk = bedrock_malloc(sizeof(struct chunk));
	chunk->column = column;
	chunk->y = y;

	/* Get the sections tag */
	sections = nbt_get(column->data, TAG_LIST, 2, "Level", "Sections");
	/* Add a new compound to the sections list for this chunk */
	compound = nbt_add(sections, TAG_COMPOUND, NULL, NULL, 0);

	nbt_add(compound, TAG_BYTE, "Y", &y, sizeof(y));
	
	/* Initialize empty arrays in NBT structures and set pointers to them */
	tag = nbt_add(compound, TAG_BYTE_ARRAY, "Data", bedrock_malloc(BEDROCK_DATA_LENGTH), BEDROCK_DATA_LENGTH);
	chunk->data = (uint8_t *) tag->payload.tag_byte_array.data;

	tag = nbt_add(compound, TAG_BYTE_ARRAY, "SkyLight", bedrock_malloc(BEDROCK_DATA_LENGTH), BEDROCK_DATA_LENGTH);
	chunk->skylight = (uint8_t *) tag->payload.tag_byte_array.data;

	tag = nbt_add(compound, TAG_BYTE_ARRAY, "BlockLight", bedrock_malloc(BEDROCK_DATA_LENGTH), BEDROCK_DATA_LENGTH);
	chunk->blocklight = (uint8_t *) tag->payload.tag_byte_array.data;

	tag = nbt_add(compound, TAG_BYTE_ARRAY, "Blocks", bedrock_malloc(BEDROCK_BLOCK_LENGTH), BEDROCK_BLOCK_LENGTH);
	chunk->blocks = (uint8_t *) tag->payload.tag_byte_array.data;

	column->chunks[y] = chunk;

	return chunk;
}

struct chunk *chunk_load(struct column *column, uint8_t y, nbt_tag *chunk_tag)
{
	struct chunk *chunk;
	struct nbt_tag_byte_array *byte_array;

	bedrock_assert(y < BEDROCK_CHUNKS_PER_COLUMN, return NULL);
	bedrock_assert(column != NULL && column->chunks[y] == NULL, return NULL);

	chunk = bedrock_malloc(sizeof(struct chunk));
	chunk->column = column;
	chunk->y = y;

	byte_array = &nbt_get(chunk_tag, TAG_BYTE_ARRAY, 1, "Blocks")->payload.tag_byte_array;
	bedrock_assert(byte_array->length == BEDROCK_BLOCK_LENGTH, ;);
	chunk->blocks = (uint8_t *) byte_array->data;

	byte_array = &nbt_get(chunk_tag, TAG_BYTE_ARRAY, 1, "Data")->payload.tag_byte_array;
	bedrock_assert(byte_array->length == BEDROCK_DATA_LENGTH, ;);
	chunk->data = (uint8_t *) byte_array->data;

	byte_array = &nbt_get(chunk_tag, TAG_BYTE_ARRAY, 1, "SkyLight")->payload.tag_byte_array;
	bedrock_assert(byte_array->length == BEDROCK_DATA_LENGTH, ;);
	chunk->skylight = (uint8_t *) byte_array->data;

	byte_array = &nbt_get(chunk_tag, TAG_BYTE_ARRAY, 1, "BlockLight")->payload.tag_byte_array;
	bedrock_assert(byte_array->length == BEDROCK_DATA_LENGTH, ;);
	chunk->blocklight = (uint8_t *) byte_array->data;

	column->chunks[y] = chunk;

	return chunk;
}

uint8_t *chunk_get_block(struct chunk *chunk, int32_t x, uint8_t y, int32_t z)
{
	uint16_t block_index;

	bedrock_assert(chunk->blocks != NULL, return NULL);
	bedrock_assert(chunk->y == y / BEDROCK_BLOCKS_PER_CHUNK, return NULL);

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

void chunk_free(struct chunk *chunk)
{
	int i;

	if (!chunk)
		return;

	bedrock_log(LEVEL_COLUMN, "chunk: Freeing chunk %d in column %d,%d", chunk->y, chunk->column->x, chunk->column->z);

	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		if (chunk->column->chunks[i] == chunk)
			chunk->column->chunks[i] = NULL;

	bedrock_free(chunk);
}

struct chunk *find_chunk_which_contains(struct world *world, int32_t x, uint8_t y, int32_t z)
{
	struct region *region;
	struct column *column;

	region = find_region_which_contains(world, x, z);
	if (region == NULL)
		return NULL;

	column = find_column_which_contains(region, x, z);
	if (column == NULL)
		return NULL;

	bedrock_assert(y / BEDROCK_BLOCKS_PER_CHUNK < BEDROCK_CHUNKS_PER_COLUMN, return NULL);
	return column->chunks[y / BEDROCK_BLOCKS_PER_CHUNK];
}
