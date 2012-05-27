#include "util/buffer.h"
#include "util/memory.h"
#include "server/bedrock.h"

#include <limits.h>

bedrock_buffer *bedrock_buffer_create(const void *data, size_t length, size_t capacity)
{
	bedrock_buffer *buffer;

	bedrock_assert(length <= capacity, return NULL);

	buffer = bedrock_malloc(sizeof(bedrock_buffer));
	buffer->data = bedrock_malloc(capacity);
	buffer->length = 0;
	buffer->capacity = capacity;

	bedrock_buffer_append(buffer, data, length);

	return buffer;
}

void bedrock_buffer_free(bedrock_buffer *buffer)
{
	bedrock_free(buffer->data);
	bedrock_free(buffer);
}

void bedrock_buffer_ensure_capacity(bedrock_buffer *buffer, size_t size)
{
	if (buffer->capacity - buffer->length < size)
	{
		size_t i, n = size + buffer->length, old = buffer->capacity;
		for (i = 1; i < sizeof(size_t) * CHAR_BIT; i <<= 1)
			n |= n >> i;
		n ^= n >> 1;
		n <<= 1;

		buffer->capacity = n;
		buffer->data = bedrock_realloc(buffer->data, buffer->capacity);

		bedrock_log(LEVEL_BUFFER, "buffer: Resizing buffer %p from %ld to %ld", buffer, old, buffer->capacity);
	}

	bedrock_assert(buffer->capacity - buffer->length >= size, return);
}

void bedrock_buffer_check_capacity(bedrock_buffer *buffer, size_t min)
{
	if (buffer->length <= buffer->capacity / 2 && buffer->capacity / 2 >= min)
	{
		size_t old = buffer->capacity;
		buffer->capacity /= 2;
		buffer->data = bedrock_realloc(buffer->data, buffer->capacity);

		bedrock_log(LEVEL_BUFFER, "buffer: Resizing buffer %p from %ld to %ld", buffer, old, buffer->capacity);
	}
}

void bedrock_buffer_append(bedrock_buffer *buffer, const void *data, size_t length)
{
	bedrock_buffer_ensure_capacity(buffer, length);
	memcpy(buffer->data + buffer->length, data, length);
	buffer->length += length;
}

void bedrock_buffer_resize(bedrock_buffer *buffer, size_t size)
{
	bedrock_assert(size >= buffer->length, return);

	buffer->capacity = size;
	buffer->data = bedrock_realloc(buffer->data, buffer->capacity);
}
