#include "util/memory.h"

long long memory_size = 0;
bedrock_spinlock memory_lock;

void bedrock_memory_init()
{
	bedrock_spinlock_init(&memory_lock, "memory spinlock");
}

void *bedrock_malloc(size_t size)
{
	void *memory = calloc(1, size + sizeof(size_t));
	size_t *sz;

	if (memory == NULL)
		abort();

	sz = memory;
	memory = sz + 1;

	*sz = size;

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

	sz = pointer;
	--sz;
	pointer = sz;

	bedrock_spinlock_lock(&memory_lock);
	memory_size -= *sz + sizeof(size_t);
	bedrock_assert(memory_size >= 0, memory_size = 0);
	bedrock_spinlock_unlock(&memory_lock);

	pointer = realloc(pointer, size + sizeof(size_t));
	if (pointer == NULL)
		abort();

	sz = pointer;
	pointer = sz + 1;

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

	sz = pointer;
	--sz;

	bedrock_spinlock_lock(&memory_lock);
	memory_size -= *sz + sizeof(size_t);
	bedrock_assert(memory_size >= 0, memory_size = 0);
	bedrock_spinlock_unlock(&memory_lock);

	free(sz);
}
