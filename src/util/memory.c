#include "util/memory.h"

struct bedrock_memory_pool bedrock_memory = BEDROCK_MEMORY_POOL_INIT("main memory pool");

void *bedrock_malloc_pool(struct bedrock_memory_pool *pool, size_t size)
{
	void *memory;

	if (pool == NULL)
		return bedrock_malloc(size);

	memory = bedrock_malloc(size);

	bedrock_mutex_lock(&pool->mutex);
	pool->size += size;
	bedrock_mutex_unlock(&pool->mutex);

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

	bedrock_mutex_lock(&pool->mutex);
	pool->size -= *sz;
	bedrock_assert(pool->size >= 0, pool->size = 0);
	bedrock_mutex_unlock(&pool->mutex);

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

	bedrock_mutex_lock(&pool->mutex);
	pool->size -= *sz;
	bedrock_assert(pool->size >= 0, pool->size = 0);
	bedrock_mutex_unlock(&pool->mutex);

	pointer = bedrock_realloc(pointer, size);

	bedrock_mutex_lock(&pool->mutex);
	pool->size += size;
	bedrock_mutex_unlock(&pool->mutex);

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

	bedrock_mutex_lock(&bedrock_memory.mutex);
	bedrock_memory.size += (int64_t) (*sz + sizeof(size_t));
	bedrock_mutex_unlock(&bedrock_memory.mutex);

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

	pointer = ((size_t *) pointer) - 1;
	sz = pointer;

	bedrock_mutex_lock(&bedrock_memory.mutex);
	bedrock_memory.size -= (int64_t) (*sz + sizeof(size_t));
	bedrock_assert(bedrock_memory.size >= 0, bedrock_memory.size = 0);
	bedrock_mutex_unlock(&bedrock_memory.mutex);

	pointer = realloc(pointer, size + sizeof(size_t));
	if (pointer == NULL)
		abort();

	sz = pointer;
	pointer = ((size_t *) pointer) + 1;

	*sz = size;

	bedrock_mutex_lock(&bedrock_memory.mutex);
	bedrock_memory.size += (int64_t) (*sz + sizeof(size_t));
	bedrock_mutex_unlock(&bedrock_memory.mutex);

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
	bedrock_mutex_lock(&bedrock_memory.mutex);
	bedrock_memory.size -= (int64_t) (*sz + sizeof(size_t));
	bedrock_assert(bedrock_memory.size >= 0, bedrock_memory.size = 0);
	bedrock_mutex_unlock(&bedrock_memory.mutex);

	free(sz);
}
