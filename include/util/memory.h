#ifndef BEDROCK_UTIL_MEMORY_H
#define BEDROCK_UTIL_MEMORY_H

#include "util.h"
#include "list.h"

#include <stdint.h>

typedef size_t bedrock_memory_pool;

#define BEDROCK_MEMORY_POOL_INIT { 0 }

extern uint64_t bedrock_memory;

extern void *bedrock_malloc_pool(struct bedrock_memory_pool *pool, size_t size);
extern void bedrock_free_pool(struct bedrock_memory_pool *pool, void *pointer);
extern void *bedrock_realloc_pool(struct bedrock_memory_pool *pool, void *pointer, size_t size);

extern void *bedrock_malloc(size_t size);
extern void *bedrock_realloc(void *pointer, size_t size);
extern void bedrock_free(void *pointer);
extern char *bedrock_strdup(const char *string);

#endif // BEDROCK_UTIL_MEMORY_H
