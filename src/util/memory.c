#include "util/memory.h"

long long memory_size = 0;

void *bedrock_malloc(size_t size)
{
	void *memory = calloc(1, size + sizeof(size_t));
	size_t *sz;

	if (memory == NULL)
		abort();

	sz = memory;
	memory = ((size_t *) memory) + 1;

	*sz = size;

	__sync_fetch_and_add(&memory_size, (*sz + sizeof(size_t)));

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

	bedrock_assert(__sync_fetch_and_sub(&memory_size, (*sz + sizeof(size_t))) >= 0, __sync_and_and_fetch(&memory_size, 0));

	pointer = realloc(pointer, size + sizeof(size_t));
	if (pointer == NULL)
		abort();

	sz = pointer;
	pointer = ((size_t *) pointer) + 1;

	*sz = size;

	__sync_fetch_and_add(&memory_size, (*sz + sizeof(size_t)));

	return pointer;
}

void bedrock_free(void *pointer)
{
	size_t *sz;

	if (pointer == NULL)
		return;

	sz = ((size_t *) pointer) - 1;
	bedrock_assert(__sync_fetch_and_sub(&memory_size, (*sz + sizeof(size_t))) >= 0, __sync_and_and_fetch(&memory_size, 0));

	free(sz);
}
