#include "util/memory.h"

long long memory_size = 0;
bedrock_spinlock memory_lock;

void *bedrock_malloc(size_t size)
{
	void *memory = calloc(1, size + sizeof(size_t));
	size_t *sz;
	static bool init = false;

	if (memory == NULL)
		abort();

	sz = memory;
	memory = ((size_t *) memory) + 1;

	*sz = size;

	if (!init)
	{
		bedrock_spinlock_init(&memory_lock, "memory spinlock");
		init = true;
	}

	bedrock_spinlock_lock(&memory_lock);
	memory_size += size + sizeof(size_t);
	bedrock_spinlock_unlock(&memory_lock);

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

	bedrock_spinlock_lock(&memory_lock);
	memory_size -= *sz + sizeof(size_t);
	bedrock_assert(memory_size >= 0, memory_size = 0);
	bedrock_spinlock_unlock(&memory_lock);

	pointer = realloc(pointer, size + sizeof(size_t));
	if (pointer == NULL)
		abort();

	sz = pointer;
	pointer = ((size_t *) pointer) + 1;

	*sz = size;

	bedrock_spinlock_lock(&memory_lock);
	memory_size += size + sizeof(size_t);
	bedrock_spinlock_unlock(&memory_lock);

	return pointer;
}

void bedrock_free(void *pointer)
{
	size_t *sz;

	if (pointer == NULL)
		return;

	sz = ((size_t *) pointer) - 1;

	bedrock_spinlock_lock(&memory_lock);
	memory_size -= *sz + sizeof(size_t);
	bedrock_assert(memory_size >= 0, memory_size = 0);
	bedrock_spinlock_unlock(&memory_lock);

	free(sz);
}
