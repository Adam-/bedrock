#include <string.h>

typedef struct
{
	unsigned char *data;
	size_t length;
	size_t capacity;
} compression_buffer;

extern void compression_compress(compression_buffer **buf, const char *data, size_t len);
extern compression_buffer *compression_decompress(const char *data, size_t len);
extern void compression_free_buffer(compression_buffer *buf);
