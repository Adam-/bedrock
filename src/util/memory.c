#include "util/memory.h"

uint64_t bedrock_memory = 0;

void *bedrock_malloc_pool(struct bedrock_memory_pool *pool, size_t size)
{
	bedrock_memory_block *block;

	if (pool == NULL)
		return bedrock_malloc(size);

	block = bedrock_malloc(sizeof(bedrock_memory_block) + size);
	block->size = size;
	block->memory = (unsigned char *) (block + 1);

	pool->size += size;
	bedrock_list_add_node(&pool->list, &block->node, block);

	return block->memory;
}

void bedrock_free_pool(struct bedrock_memory_pool *pool, void *ptr)
{
	bedrock_memory_block *block;

	if (pool == NULL)
	{
		bedrock_free(ptr);
		return;
	}

	if (ptr == NULL)
		return;

	block = ((bedrock_memory_block *) ptr) - 1;
	bedrock_assert(block->memory == ptr, return);

	pool->size -= block->size;
	bedrock_list_del_node(&pool->list, &block->node);

	bedrock_free(block);
}

void *bedrock_realloc_pool(struct bedrock_memory_pool *pool, void *pointer, size_t size)
{
	bedrock_memory_block *block;

	if (pool == NULL)
		return bedrock_realloc(pointer, size);
	else if (pointer == NULL)
		return bedrock_malloc_pool(pool, size);
	else if (size == 0)
	{
		bedrock_free_pool(pool, pointer);
		return NULL;
	}

	block = ((bedrock_memory_block *) pointer) - 1;
	bedrock_assert(block->memory == pointer, return NULL);

	pool->size -= block->size;

	block = bedrock_realloc(block, sizeof(bedrock_memory_block) + size);
	if (block == NULL)
		abort();

	pool->size += size;

	block->size = size;
	block->memory = (unsigned char *) (block + 1);

	return block->memory;
}

#if 0
void *bedrock_malloc_pool(bedrock_memory_pool *pool, size_t size)
{
	return bedrock_malloc(size);
}

void bedrock_free_pool(bedrock_memory_pool *pool, void *ptr)
{
	bedrock_free(ptr);
}

void *bedrock_realloc_pool(bedrock_memory_pool *pool, void *pointer, size_t size)
{
	return bedrock_realloc(pointer, size);
}
#endif

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
