#include "server/bedrock.h"
#include "compression/compression.h"
#include "util/util.h"
#include <zlib.h>

bedrock_buffer *compression_compress(const char *data, size_t len)
{
	bedrock_buffer *buf;
	int i;

	z_stream stream = {
			.zalloc = Z_NULL,
			.zfree = Z_NULL,
			.opaque = Z_NULL,
			.next_in = data,
			.avail_in = len
	};

	i = deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY);
	if (i != Z_OK)
	{
		bedrock_log(LEVEL_CRIT, "zlib: Error initializing deflate stream - error code %d", i);
		return;
	}

	bedrock_assert_ret(stream.avail_in == len, false);

	buf = bedrock_buffer_create(NULL, 0, BEDROCK_BUFFER_DEFAULT_SIZE);

	do
	{
		bedrock_ensure_capacity(buf, BEDROCK_BUFFER_DEFAULT_SIZE);

		bedrock_assert_do(buf->capacity - buf->length >= BEDROCK_BUFFER_DEFAULT_SIZE, break);

		stream.next_out = buf->data + buf->length;
		stream.avail_out = BEDROCK_BUFFER_DEFAULT_SIZE;

		i = deflate(&stream, Z_FINISH);
		if (i == Z_OK || i == Z_STREAM_END)
			buf->length += BEDROCK_BUFFER_DEFAULT_SIZE - stream.avail_out;
	}
	while (i == Z_OK);

	deflateEnd(&stream);

	if (i != Z_STREAM_END)
	{
		bedrock_log(LEVEL_CRIT, "zlib: Error deflating stream - error code %d", i);
		compression_free_buffer(buf);
		buf = NULL;
	}

	return buf;
}

bedrock_buffer *compression_decompress(const char *data, size_t len)
{
	bedrock_buffer *buf;
	int i;

	bedrock_assert_ret(len > 0, NULL);

	z_stream stream = {
			.zalloc = Z_NULL,
			.zfree = Z_NULL,
			.opaque = Z_NULL,
			.next_in = data,
			.avail_in = len
	};

	bedrock_assert_ret(stream.avail_in == len, false);

	i = inflateInit2(&stream, 15 + 32);
	if (i != Z_OK)
	{
		bedrock_log(LEVEL_CRIT, "zlib: Error initializing inflate stream - error code %d", i);
		return NULL;
	}

	buf = bedrock_buffer_create(NULL, 0, len);

	do
	{
		bedrock_ensure_capacity(buf, BEDROCK_BUFFER_DEFAULT_SIZE);

		bedrock_assert_do(buf->capacity - buf->length >= BEDROCK_BUFFER_DEFAULT_SIZE, break);

		stream.next_out = buf->data + buf->length;
		stream.avail_out = BEDROCK_BUFFER_DEFAULT_SIZE;

		i = inflate(&stream, Z_NO_FLUSH);
		if (i == Z_OK || i == Z_STREAM_END)
			buf->length += BEDROCK_BUFFER_DEFAULT_SIZE - stream.avail_out;
	}
	while (i == Z_OK);

	inflateEnd(&stream);

	if (i != Z_OK && i != Z_STREAM_END)
	{
		bedrock_log(LEVEL_CRIT, "zlib: Error inflating stream - error code %d", i);
		compression_free_buffer(buf);
		return NULL;
	}

	return buf;
}

void compression_free_buffer(bedrock_buffer *buf)
{
	bedrock_free(buf->data);
	bedrock_free(buf);
}
