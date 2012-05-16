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
} compression_buffer;

extern compression_buffer *compression_compress_init();
extern void compression_compress_end(compression_buffer *buffer);
extern void compression_compress_deflate(compression_buffer *buffer, const char *data, size_t len);

extern compression_buffer *compression_decompress_init();
extern void compression_decompress_end(compression_buffer *buffer);
extern void compression_decompress_inflate(compression_buffer *buffer, const char *data, size_t len);

extern compression_buffer *compression_compress(const char *data, size_t len);
extern compression_buffer *compression_decompress(const char *data, size_t len);

