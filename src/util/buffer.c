#include "util/buffer.h"
#include "server/bedrock.h"

#include <limits.h>

bedrock_buffer *bedrock_buffer_create(struct bedrock_memory_pool *pool, const void *data, size_t length, size_t capacity)
{
	bedrock_buffer *buffer;

	bedrock_assert(length <= capacity, return NULL);

	buffer = bedrock_malloc_pool(pool, sizeof(bedrock_buffer));
	buffer->pool = pool;
	buffer->data = bedrock_malloc_pool(pool, capacity);
	buffer->length = 0;
	buffer->capacity = capacity;

	bedrock_buffer_append(buffer, data, length);

	return buffer;
}

void bedrock_buffer_free(bedrock_buffer *buffer)
{
	if (buffer == NULL)
		return;
	bedrock_free_pool(buffer->pool, buffer->data);
	bedrock_free_pool(buffer->pool, buffer);
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
		buffer->data = bedrock_realloc_pool(buffer->pool, buffer->data, buffer->capacity);

		bedrock_log(LEVEL_BUFFER, "buffer: Resizing buffer %p from %ld to %ld", buffer, old, buffer->capacity);
	}

	bedrock_assert(buffer->capacity - buffer->length >= size, return);
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
	buffer->data = bedrock_realloc_pool(buffer->pool, buffer->data, buffer->capacity);
}
