#include "server/column.h"
#include "nbt/nbt.h"
#include "util/memory.h"
#include "compression/compression.h"

#define DATA_CHUNK_SIZE 8192

struct bedrock_column *column_create(struct bedrock_region *region, nbt_tag *data)
{
	struct bedrock_column *column = bedrock_malloc(sizeof(struct bedrock_column));
	nbt_tag *tag;
	bedrock_node *node;
	struct nbt_tag_byte_array *byte_array;
	compression_buffer *buffer = compression_compress_init(DATA_CHUNK_SIZE);

	column->region = region;
	nbt_copy(data, TAG_INT, &column->x, sizeof(column->x), 2, "Level", "xPos");
	nbt_copy(data, TAG_INT, &column->z, sizeof(column->z), 2, "Level", "zPos");
	column->data = data;

	tag = nbt_get(data, TAG_LIST, 2, "Level", "Sections");
	LIST_FOREACH(&tag->payload.tag_list, node)
	{
		nbt_tag *chunk_tag = node->data;
		uint8_t y;
		struct bedrock_chunk *chunk;

		nbt_copy(chunk_tag, TAG_BYTE, &y, sizeof(y), 1, "Y");
		bedrock_assert(y < sizeof(column->chunks) / sizeof(struct bedrock_chunk *), continue);

		bedrock_assert(column->chunks[y] == NULL, continue);

		chunk = column->chunks[y] = chunk_create(column, y);

		byte_array = &nbt_get(chunk_tag, TAG_BYTE_ARRAY, 1, "Data")->payload.tag_byte_array;
		bedrock_assert(byte_array->length == 2048, ;);
		compression_compress_deflate(buffer, byte_array->data, byte_array->length);
		chunk->compressed_data = buffer->buffer;
		buffer->buffer = NULL;
		compression_compress_reset(buffer);
		bedrock_buffer_resize(chunk->compressed_data, chunk->compressed_data->length);

		byte_array = &nbt_get(chunk_tag, TAG_BYTE_ARRAY, 1, "SkyLight")->payload.tag_byte_array;
		bedrock_assert(byte_array->length == 2048, ;);
		compression_compress_deflate(buffer, byte_array->data, byte_array->length);
		chunk->compressed_skylight = buffer->buffer;
		buffer->buffer = NULL;
		compression_compress_reset(buffer);
		bedrock_buffer_resize(chunk->compressed_skylight, chunk->compressed_skylight->length);

		byte_array = &nbt_get(chunk_tag, TAG_BYTE_ARRAY, 1, "BlockLight")->payload.tag_byte_array;
		bedrock_assert(byte_array->length == 2048, ;);
		compression_compress_deflate(buffer, byte_array->data, byte_array->length);
		chunk->compressed_blocklight = buffer->buffer;
		buffer->buffer = NULL;
		compression_compress_reset(buffer);
		bedrock_buffer_resize(chunk->compressed_blocklight, chunk->compressed_blocklight->length);

		byte_array = &nbt_get(chunk_tag, TAG_BYTE_ARRAY, 1, "Blocks")->payload.tag_byte_array;
		bedrock_assert(byte_array->length == 4096, ;);
		compression_compress_deflate(buffer, byte_array->data, byte_array->length);
		chunk->compressed_blocks = buffer->buffer;
		buffer->buffer = NULL;
		compression_compress_reset(buffer);
		bedrock_buffer_resize(chunk->compressed_blocks, chunk->compressed_blocks->length);
	}

	nbt_free(tag);

	tag = nbt_get(data, TAG_BYTE_ARRAY, 2, "Level", "Biomes");
	byte_array = &tag->payload.tag_byte_array;
	bedrock_assert(byte_array->length == 256, ;);
	compression_compress_deflate(buffer, byte_array->data, byte_array->length);
	column->compressed_biomes = buffer->buffer;
	buffer->buffer = NULL;
	compression_compress_end(buffer);
	bedrock_buffer_resize(column->compressed_biomes, column->compressed_biomes->length);
	nbt_free(tag);

	return column;
}

void column_free(struct bedrock_column *column)
{
	int i;
	for (i = 0; i < BEDROCK_CHUNKS_PER_COLUMN; ++i)
		chunk_free(column->chunks[i]);

	bedrock_buffer_free(column->compressed_biomes);

	nbt_free(column->data);
	bedrock_free(column);
}
