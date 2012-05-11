#include "util/memory.h"

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

