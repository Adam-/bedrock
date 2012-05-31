#include "util/buffer.h"
#include <zlib.h>

typedef enum
{
	ZLIB_COMPRESS,
	ZLIB_DECOMPRESS
} compression_stream_type;

typedef struct
{
	bedrock_buffer *buffer;
	compression_stream_type type;
	z_stream stream;
	size_t buffer_size;
	int decompress_type;
} compression_buffer;

#define ZLIB_HEADER_SIZE 2

extern compression_buffer *compression_compress_init_type(size_t buffer_size, int type);
extern compression_buffer *compression_compress_init(size_t buffer_size);
extern void compression_compress_end(compression_buffer *buffer);
extern void compression_compress_deflate(compression_buffer *buffer, const char *data, size_t len);

extern compression_buffer *compression_decompress_init(size_t buffer_size);
extern void compression_decompress_end(compression_buffer *buffer);
extern void compression_decompress_reset(compression_buffer *buffer);
extern void compression_decompress_inflate(compression_buffer *buffer, const char *data, size_t len);

extern compression_buffer *compression_compress(size_t buffer_size, const char *data, size_t len);
extern compression_buffer *compression_decompress(size_t buffer_size, const char *data, size_t len);

