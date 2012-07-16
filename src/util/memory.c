#include "util/memory.h"

bedrock_mutex memory_mutex = BEDROCK_MUTEX_INIT("memory mutex");
int64_t memory_size = 0;

void *bedrock_malloc(size_t size)
{
	void *memory = calloc(1, size + sizeof(size_t));
	size_t *sz;

	if (memory == NULL)
		abort();

	sz = memory;
	memory = ((size_t *) memory) + 1;

	*sz = size;

	bedrock_mutex_lock(&memory_mutex);
	memory_size += (int64_t) (*sz + sizeof(size_t));
	bedrock_mutex_unlock(&memory_mutex);

	return memory;
}

void *bedrock_realloc(void *pointer, size_t size)
{
	size_t *sz;

	if (pointer == NULL)
		return bedrock_malloc(size);
	else if (size == 0)
	{
		bedrock_free(pointer);
		return NULL;
	}

	pointer = ((size_t *) pointer) - 1;
	sz = pointer;

	bedrock_mutex_lock(&memory_mutex);
	memory_size -= (int64_t) (*sz + sizeof(size_t));
	bedrock_assert(memory_size >= 0, memory_size = 0);
	bedrock_mutex_unlock(&memory_mutex);

	pointer = realloc(pointer, size + sizeof(size_t));
	if (pointer == NULL)
		abort();

	sz = pointer;
	pointer = ((size_t *) pointer) + 1;

	*sz = size;

	bedrock_mutex_lock(&memory_mutex);
	memory_size += (int64_t) (*sz + sizeof(size_t));
	bedrock_mutex_unlock(&memory_mutex);

	return pointer;
}

void bedrock_free(void *pointer)
{
	size_t *sz;

	if (pointer == NULL)
		return;

	sz = ((size_t *) pointer) - 1;
	bedrock_mutex_lock(&memory_mutex);
	memory_size -= (int64_t) (*sz + sizeof(size_t));
	bedrock_assert(memory_size >= 0, memory_size = 0);
	bedrock_mutex_unlock(&memory_mutex);

	free(sz);
}
