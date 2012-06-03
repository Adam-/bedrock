#include "util/memory.h"

uint64_t bedrock_memory = 0;

void *bedrock_malloc_pool(struct bedrock_memory_pool *pool, size_t size)
{
	void *memory;

	if (pool == NULL)
		return bedrock_malloc(size);

	memory = bedrock_malloc(size);
	pool->size += size;

	return memory;
}

void bedrock_free_pool(struct bedrock_memory_pool *pool, void *pointer)
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
	pool->size -= *sz;

	bedrock_free(pointer);
}

void *bedrock_realloc_pool(struct bedrock_memory_pool *pool, void *pointer, size_t size)
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
	pool->size -= *sz;

	pointer = bedrock_realloc(pointer, size);
	if (pointer == NULL)
		abort();

	pool->size += size;

	return pointer;
}

/** Allocate memory
 * @param size The size of memory to allocate
 * @return Memory initialized to 0
 */
void *bedrock_malloc(size_t size)
{
	void *memory = calloc(1, size + sizeof(size_t));
	size_t *sz = memory;
	memory = ((size_t *) memory) + 1;
	if (!memory)
		abort();
	*sz = size + sizeof(size_t);
	bedrock_memory += size + sizeof(size_t);
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

	sz = pointer = ((size_t *) pointer) - 1;
	bedrock_memory -= *sz;

	pointer = realloc(pointer, size + sizeof(size_t));
	if (pointer == NULL)
		abort();

	sz = pointer;
	*sz = size;
	bedrock_memory += size;
	pointer = ((size_t *) pointer) + 1;

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
	bedrock_memory -= *sz;

	free(sz);
}
