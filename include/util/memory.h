#ifndef BEDROCK_UTIL_MEMORY_H
#define BEDROCK_UTIL_MEMORY_H

#include "util.h"
#include "thread.h"

struct bedrock_memory_pool
{
	bedrock_mutex mutex;
	int64_t size;
};

#include "list.h"

#define BEDROCK_MEMORY_POOL_INIT(desc) { BEDROCK_MUTEX_INIT(desc), 0 }

extern struct bedrock_memory_pool bedrock_memory;

extern void *bedrock_malloc_pool(struct bedrock_memory_pool *pool, size_t size);
extern void bedrock_free_pool(struct bedrock_memory_pool *pool, void *pointer);
extern void *bedrock_realloc_pool(struct bedrock_memory_pool *pool, void *pointer, size_t size);

extern void *bedrock_malloc(size_t size);
extern void *bedrock_realloc(void *pointer, size_t size);
extern void bedrock_free(void *pointer);
extern char *bedrock_strdup(const char *string);

#endif // BEDROCK_UTIL_MEMORY_H
