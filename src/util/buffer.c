#include "util/buffer.h"
#include "server/bedrock.h"

#include <limits.h>

bedrock_buffer *bedrock_buffer_create(const char *name, const void *data, size_t length, size_t capacity)
{
	bedrock_buffer *buffer;

	bedrock_assert(length <= capacity, return NULL);

	buffer = bedrock_malloc(sizeof(bedrock_buffer));
	strncpy(buffer->name, name, sizeof(buffer->name));
	buffer->data = bedrock_malloc(capacity);
	buffer->length = 0;
	buffer->capacity = capacity;

	bedrock_buffer_append(buffer, data, length);

	return buffer;
}

void bedrock_buffer_free(bedrock_buffer *buffer)
{
	if (buffer == NULL)
		return;
	bedrock_free(buffer->data);
	bedrock_free(buffer);
}

void bedrock_buffer_ensure_capacity(bedrock_buffer *buffer, size_t size)
{
	while (buffer->capacity - buffer->length < size)
	{
		size_t old = buffer->capacity;

		if (buffer->capacity != 0)
			buffer->capacity *= 2;
		else
			buffer->capacity = size;
		buffer->data = bedrock_realloc(buffer->data, buffer->capacity);

		bedrock_log(LEVEL_BUFFER, "buffer: Resizing buffer %s from %ld to %ld", buffer->name, old, buffer->capacity);
	}

	bedrock_assert(buffer->capacity - buffer->length >= size, ;);
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
