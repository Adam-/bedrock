#include "server/bedrock.h"
#include "compression/compression.h"
#include "util/util.h"
#include <zlib.h>

#define CHUNK_SIZE 4096

compression_buffer *compression_decompress(const char *data, size_t len)
{
	compression_buffer *buf;
	int i;

	bedrock_assert_ret(len > 0, NULL);

	z_stream stream = {
			.zalloc = Z_NULL,
			.zfree = Z_NULL,
			.opaque = Z_NULL,
			.next_in = data,
			.avail_in = len
	};

	i = inflateInit2(&stream, 15 + 32);
	if (i != Z_OK)
	{
		bedrock_log(LEVEL_CRIT, "zlib: Error initializing inflate stream - error code %d", i);
		return NULL;
	}

	buf = bedrock_malloc(sizeof(compression_buffer));
	buf->data = bedrock_malloc(len);
	buf->length = 0;
	buf->capacity = len;

	do
	{
		while (buf->capacity - buf->length < CHUNK_SIZE)
		{
			buf->capacity *= 2;
			buf->data = bedrock_realloc(buf->data, buf->capacity);
		}

		bedrock_assert_do(buf->capacity - buf->length >= CHUNK_SIZE, break);

		stream.next_out = buf->data + buf->length;
		stream.avail_out = CHUNK_SIZE;

		i = inflate(&stream, Z_NO_FLUSH);
		if (i != Z_OK && i != Z_STREAM_END)
		{
			bedrock_log(LEVEL_CRIT, "zlib: Error inflating stream - error code %d", i);
			compression_free_buffer(buf);
			return NULL;
		}

		buf->length += CHUNK_SIZE - stream.avail_out;
	}
	while (i == Z_OK);

	inflateEnd(&stream);

	if (i != Z_STREAM_END)
	{
		bedrock_log(LEVEL_CRIT, "zlib: EOF reached but not end of stream?");
		compression_free_buffer(buf);
		return NULL;
	}

	return buf;
}

void compression_free_buffer(compression_buffer *buf)
{
	bedrock_free(buf->data);
	bedrock_free(buf);
}
