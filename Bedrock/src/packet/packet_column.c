#include "server/client.h"
#include "packet/packet.h"
#include "compression/compression.h"

void packet_send_column(struct bedrock_client *client, nbt_tag *column)
{
	uint8_t b;
	int8_t last_y = -1;
	uint16_t bitmask;
	nbt_tag *tag;
	bedrock_node *node;
	compression_buffer *buffer;
	struct nbt_tag_byte_array *blocks;

	client_send_header(client, MAP_COLUMN);
	client_send_int(client, nbt_read(column, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(column, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = 1;
	client_send_int(client, &b, sizeof(b)); // Ground up continuous

	tag = nbt_get(column, 2, "Level", "Sections");
	bedrock_assert_ret(tag != NULL && tag->type == TAG_LIST, ERROR_UNKNOWN);
	bitmask = 0;
	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		nbt_copy(sec, &b, sizeof(b), 1, "Y");
		bedrock_assert_ret((last_y + 1) == b, ERROR_UNKNOWN);
		last_y = b;
		bitmask |= 1 << b;
	}
	client_send_int(client, &bitmask, sizeof(bitmask)); // primary bit map

	bitmask = 0;
	client_send_int(client, &bitmask, sizeof(bitmask)); // add bit map

	buffer = compression_compress_init();
	bedrock_assert_ret(buffer, ERROR_UNKNOWN);

	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "Blocks");
		bedrock_assert_ret(blocks->length == 4096, ERROR_UNKNOWN);

		compression_compress_deflate(buffer, blocks->data, blocks->length);
	}

	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "Data");
		bedrock_assert_ret(blocks->length == 2048, ERROR_UNKNOWN);

		compression_compress_deflate(buffer, blocks->data, blocks->length);
	}

	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "BlockLight");
		bedrock_assert_ret(blocks->length == 2048, ERROR_UNKNOWN);

		compression_compress_deflate(buffer, blocks->data, blocks->length);
	}

	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "SkyLight");
		bedrock_assert_ret(blocks->length == 2048, ERROR_UNKNOWN);

		compression_compress_deflate(buffer, blocks->data, blocks->length);
	}

	blocks = nbt_read(column, TAG_BYTE_ARRAY, 2, "Level", "Biomes");
	bedrock_assert_ret(blocks->length == 256, ERROR_UNKNOWN);

	compression_compress_deflate(buffer, blocks->data, blocks->length);

	uint32_t ii = buffer->buffer->length;
	client_send_int(client, &ii, sizeof(ii)); // length
	ii = 0;
	client_send_int(client, &ii, sizeof(ii)); // not used

	client_send(client, buffer->buffer->data, buffer->buffer->length);

	compression_compress_end(buffer);
}

void packet_send_column_empty(struct bedrock_client *client, nbt_tag *column)
{
	uint8_t b;
	uint16_t s;
	uint32_t i;

	client_send_header(client, MAP_COLUMN);
	client_send_int(client, nbt_read(column, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(column, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = 1;
	client_send_int(client, &b, sizeof(b)); // Ground up continuous

	s = 0;
	client_send_int(client, &s, sizeof(s)); // Primary bitmap
	client_send_int(client, &s, sizeof(s)); // Add bitmap

	i = 0;
	client_send_int(client, &i, sizeof(i)); // Size
	client_send_int(client, &i, sizeof(i)); // Unused
}
