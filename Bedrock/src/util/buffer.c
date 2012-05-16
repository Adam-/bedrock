#include "util/buffer.h"
#include "util/memory.h"

bedrock_buffer *bedrock_buffer_create(const void *data, size_t length, size_t capacity)
{
	bedrock_buffer *buffer;

	bedrock_assert_ret(length <= capacity, NULL);

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

void bedrock_ensure_capacity(bedrock_buffer *buffer, size_t size)
{
	if (buffer->capacity - buffer->length < size)
	{
		size_t i, n = size | buffer->capacity;
		for (i = 1; i < sizeof(size_t) * 8; i <<= 1)
			n |= n >> i;
		n ^= n >> 1;
		n <<= 1;

		buffer->capacity = n;
		buffer->data = bedrock_realloc(buffer->data, buffer->capacity);

	}
}

void bedrock_buffer_append(bedrock_buffer *buffer, const void *data, size_t length)
{
	bedrock_ensure_capacity(buffer, length);
	memcpy(buffer->data + buffer->length, data, length);
}
