#include "util/memory.h"

int64_t bedrock_memory = 0;

void *bedrock_malloc_pool(bedrock_memory_pool *pool, size_t size)
{
	void *memory;

	if (pool == NULL)
		return bedrock_malloc(size);

	memory = bedrock_malloc(size);
	*pool += size;

	return memory;
}

void bedrock_free_pool(bedrock_memory_pool *pool, void *pointer)
{
	size_t *sz;

	if (pool == NULL)
	{
		bedrock_free(pointer);
		return;
	}

	if (pointer == NULL)
		return;

	sz = ((size_t *) pointer) - 1;
	*pool -= *sz;
	if (*pool < 0)
		printf(":/\n");
	bedrock_assert(*pool >= 0, *pool = 0);

	bedrock_free(pointer);
}

void *bedrock_realloc_pool(bedrock_memory_pool *pool, void *pointer, size_t size)
{
	size_t *sz;

	if (pool == NULL)
		return bedrock_realloc(pointer, size);
	else if (pointer == NULL)
		return bedrock_malloc_pool(pool, size);
	else if (size == 0)
	{
		bedrock_free_pool(pool, pointer);
		return NULL;
	}

	sz = ((size_t *) pointer) - 1;
	*pool -= *sz;
	bedrock_assert(*pool >= 0, *pool = 0);

	pointer = bedrock_realloc(pointer, size);

	*pool += size;

	return pointer;
}

/** Allocate memory
 * @param size The size of memory to allocate
 * @return Memory initialized to 0
 */
void *bedrock_malloc(size_t size)
{
	void *memory = calloc(1, size + sizeof(size_t));
	size_t *sz;

	if (memory == NULL)
		abort();

	sz = memory;
	memory = ((size_t *) memory) + 1;

	*sz = size;
	bedrock_memory += *sz + sizeof(size_t);

	return memory;
}

/** Reallocate memory
 * @return The reallocate memory, can be uninitialized
 */
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

	sz = pointer;
	pointer = ((size_t *) pointer) - 1;

	bedrock_memory -= *sz + sizeof(size_t);

	pointer = realloc(pointer, size + sizeof(size_t));
	if (pointer == NULL)
		abort();

	sz = pointer;
	pointer = ((size_t *) pointer) + 1;

	*sz = size;
	bedrock_memory += *sz + sizeof(size_t);

	return pointer;
}

/** Free memory
 */
void bedrock_free(void *pointer)
{
	size_t *sz;

	if (pointer == NULL)
		return;

	sz = ((size_t *) pointer) - 1;
	bedrock_memory -= *sz + sizeof(size_t);

	free(sz);
}
