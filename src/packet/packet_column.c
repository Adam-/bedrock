#include "server/client.h"
#include "packet/packet.h"
#include "server/column.h"
#include "compression/compression.h"
#include "nbt/nbt.h"

#define COLUMN_BUFFER_SIZE 8192

void packet_send_column(struct bedrock_client *client, struct bedrock_column *column)
{
	uint8_t b;
	uint16_t bitmask;
	nbt_tag *tag;
	bedrock_node *node;
	compression_buffer *buffer;
	const struct nbt_tag_byte_array *blocks;

	client_send_header(client, MAP_COLUMN);
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = 1;
	client_send_int(client, &b, sizeof(b)); // Ground up continuous

	tag = nbt_get(column->data, TAG_LIST, 2, "Level", "Sections");
	bedrock_assert(tag != NULL && tag->type == TAG_LIST, return);
	bitmask = 0;
	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		nbt_copy(sec, TAG_BYTE, &b, sizeof(b), 1, "Y");
		bitmask |= 1 << b;
	}
	client_send_int(client, &bitmask, sizeof(bitmask)); // primary bit map

	bitmask = 0;
	client_send_int(client, &bitmask, sizeof(bitmask)); // add bit map

	buffer = compression_compress_init(COLUMN_BUFFER_SIZE);
	bedrock_assert(buffer, return);

	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "Blocks");
		bedrock_assert(blocks->length == 4096, goto error);

		compression_compress_deflate(buffer, blocks->data, blocks->length);
	}

	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "Data");
		bedrock_assert(blocks->length == 2048, goto error);

		compression_compress_deflate(buffer, blocks->data, blocks->length);
	}

	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "BlockLight");
		bedrock_assert(blocks->length == 2048, goto error);

		compression_compress_deflate(buffer, blocks->data, blocks->length);
	}

	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "SkyLight");
		bedrock_assert(blocks->length == 2048, goto error);

		compression_compress_deflate(buffer, blocks->data, blocks->length);
	}

	blocks = nbt_read(column->data, TAG_BYTE_ARRAY, 2, "Level", "Biomes");
	bedrock_assert(blocks->length == 256, goto error);

	compression_compress_deflate(buffer, blocks->data, blocks->length);

	uint32_t ii = buffer->buffer->length;
	client_send_int(client, &ii, sizeof(ii)); // length
	ii = 0;
	client_send_int(client, &ii, sizeof(ii)); // not used

	client_send(client, buffer->buffer->data, buffer->buffer->length);

 error:
	compression_compress_end(buffer);
}

void packet_send_column_empty(struct bedrock_client *client, struct bedrock_column *column)
{
	uint8_t b;
	uint16_t s;
	uint32_t i;

	client_send_header(client, MAP_COLUMN);
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(column->data, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = 1;
	client_send_int(client, &b, sizeof(b)); // Ground up continuous

	s = 0;
	client_send_int(client, &s, sizeof(s)); // Primary bitmap
	client_send_int(client, &s, sizeof(s)); // Add bitmap

	i = 0;
	client_send_int(client, &i, sizeof(i)); // Size
	client_send_int(client, &i, sizeof(i)); // Unused
}
