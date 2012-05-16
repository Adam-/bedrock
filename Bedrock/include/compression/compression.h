#include "util/buffer.h"

extern bedrock_buffer *compression_compress(const char *data, size_t len);
extern bedrock_buffer *compression_decompress(const char *data, size_t len);
extern void compression_free_buffer(bedrock_buffer *buf);
