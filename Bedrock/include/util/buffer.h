#ifndef BEDROCK_UTIL_BUFFER_H
#define BEDROCK_UTIL_BUFFER_H

#include <string.h>

typedef struct
{
	unsigned char *data;
	size_t length;
	size_t capacity;
} bedrock_buffer;

#define BEDROCK_BUFFER_DEFAULT_SIZE 4096

extern bedrock_buffer *bedrock_buffer_create(const void *data, size_t length, size_t capacity);
extern void bedrock_buffer_free(bedrock_buffer *buffer);
extern void bedrock_ensure_capacity(bedrock_buffer *buffer, size_t size);
extern void bedrock_buffer_append(bedrock_buffer *buffer, const void *data, size_t length);
extern void bedrock_buffer_resize(bedrock_buffer *buffer, size_t size);

#endif // BEDROCK_UTIL_BUFFER_H
