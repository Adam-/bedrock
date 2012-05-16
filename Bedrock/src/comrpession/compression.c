#include "server/bedrock.h"
#include "compression/compression.h"
#include "util/util.h"
#include "util/memory.h"

compression_buffer *compression_compress_init()
{
	compression_buffer *buffer = bedrock_malloc(sizeof(compression_buffer));

	buffer->type = ZLIB_COMPRESS;

	buffer->stream.zalloc = Z_NULL;
	buffer->stream.zfree = Z_NULL;
	buffer->stream.opaque = Z_NULL;

	int i = deflateInit2(&buffer->stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15, 8, Z_DEFAULT_STRATEGY);
	if (i != Z_OK)
	{
		bedrock_log(LEVEL_CRIT, "zlib: Error initializing deflate stream - error code %d", i);
		bedrock_free(buffer);
		return NULL;
	}

	buffer->buffer = bedrock_buffer_create(NULL, 0, BEDROCK_BUFFER_DEFAULT_SIZE);

	return buffer;
}

void compression_compress_end(compression_buffer *buffer)
{
	bedrock_assert(buffer->type == ZLIB_COMPRESS);
	deflateEnd(&buffer->stream);
	bedrock_buffer_free(buffer->buffer);
	bedrock_free(buffer);
}

void compression_compress_deflate(compression_buffer *buffer, const char *data, size_t len)
{
	int i;
	z_stream *stream;

	bedrock_assert(buffer != NULL && buffer->type == ZLIB_COMPRESS && data != NULL && len > 0);

	stream = &buffer->stream;

	stream->next_in = data;
	stream->avail_in = len;

	bedrock_assert(stream->avail_in == len);

	do
	{
		bedrock_buffer *buf = buffer->buffer;

		bedrock_ensure_capacity(buf, BEDROCK_BUFFER_DEFAULT_SIZE);

		stream->next_out = buf->data + buf->length;
		stream->avail_out = BEDROCK_BUFFER_DEFAULT_SIZE;

		i = deflate(stream, Z_BLOCK);
		if (i == Z_OK || i == Z_STREAM_END)
			buf->length += BEDROCK_BUFFER_DEFAULT_SIZE - buffer->stream.avail_out;
	}
	while (stream->avail_in > 0);

	if (i != Z_OK)
		bedrock_log(LEVEL_CRIT, "zlib: Error deflating stream - error code %d", i);
}

compression_buffer *compression_decompress_init()
{
	compression_buffer *buffer = bedrock_malloc(sizeof(compression_buffer));

	buffer->type = ZLIB_DECOMPRESS;

	buffer->stream.zalloc = Z_NULL;
	buffer->stream.zfree = Z_NULL;
	buffer->stream.opaque = Z_NULL;

	int i = inflateInit2(&buffer->stream, 15 + 32);
	if (i != Z_OK)
	{
		bedrock_log(LEVEL_CRIT, "zlib: Error initializing deflate stream - error code %d", i);
		bedrock_free(buffer);
		return NULL;
	}

	buffer->buffer = bedrock_buffer_create(NULL, 0, BEDROCK_BUFFER_DEFAULT_SIZE);

	return buffer;
}

void compression_decompress_end(compression_buffer *buffer)
{
	bedrock_assert(buffer->type == ZLIB_DECOMPRESS);
	inflateEnd(&buffer->stream);
	bedrock_buffer_free(buffer->buffer);
	bedrock_free(buffer);
}

void compression_decompress_inflate(compression_buffer *buffer, const char *data, size_t len)
{
	int i;
	z_stream *stream;

	bedrock_assert(buffer != NULL && buffer->type == ZLIB_DECOMPRESS && data != NULL && len > 0);

	stream = &buffer->stream;

	stream->next_in = data;
	stream->avail_in = len;

	bedrock_assert(stream->avail_in == len);

	do
	{
		bedrock_buffer *buf = buffer->buffer;

		bedrock_ensure_capacity(buf, BEDROCK_BUFFER_DEFAULT_SIZE);

		stream->next_out = buf->data + buf->length;
		stream->avail_out = BEDROCK_BUFFER_DEFAULT_SIZE;

		i = inflate(stream, Z_BLOCK);
		if (i == Z_OK || i == Z_STREAM_END)
			buf->length += BEDROCK_BUFFER_DEFAULT_SIZE - stream->avail_out;
	}
	while (i == Z_OK);

	if (i != Z_OK && i != Z_STREAM_END)
		bedrock_log(LEVEL_CRIT, "zlib: Error inflating stream - error code %d", i);
}

compression_buffer *compression_compress(const char *data, size_t len)
{
	compression_buffer *buffer = compression_compress_init();
	compression_compress_deflate(buffer, data, len);
	return buffer;
}

compression_buffer *compression_decompress(const char *data, size_t len)
{
	compression_buffer *buffer = compression_decompress_init();
	compression_decompress_inflate(buffer, data, len);
	return buffer;
}
