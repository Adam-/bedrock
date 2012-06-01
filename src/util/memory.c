#include "util/memory.h"

void *bedrock_malloc_pool(bedrock_memory_pool *pool, size_t size)
{
	bedrock_memory_block *block;

	bedrock_assert(pool != NULL && size > 0, return NULL);

	block = bedrock_malloc(sizeof(bedrock_memory_block) + size);
	block->size = size;
	block->memory = block + 1;

	pool->size += size;
	bedrock_list_add_node(&pool->list, &block->node, block);

	return block->memory;
}

void bedrock_free_pool(bedrock_memory_pool *pool, void *ptr)
{
	bedrock_memory_block *block;

	bedrock_assert(pool != NULL, return);

	if (ptr == NULL)
		return;

	block = ((bedrock_memory_block *) ptr) - 1;
	bedrock_assert(block->memory == ptr, return);

	pool->size -= block->size;
	bedrock_list_del_node(&pool->list, &block->node);

	bedrock_free(block);
}

/** Allocate memory
 * @param size The size of memory to allocate
 * @return Memory initialized to 0
 */
void *bedrock_malloc(size_t size)
{
	void *memory = calloc(1, size);
	if (!memory)
		abort();
	return memory;
}

/** Reallocate memory
 * @return The reallocate memory, can be uninitialized
 */
void *bedrock_realloc(void *pointer, size_t size)
{
	pointer = realloc(pointer, size);
	if (!pointer && size)
		abort();
	return pointer;
}

/** Free memory
 */
void bedrock_free(void *pointer)
{
	if (pointer)
		free(pointer);
}

