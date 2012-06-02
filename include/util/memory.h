#ifndef BEDROCK_UTIL_MEMORY_H
#define BEDROCK_UTIL_MEMORY_H

#include "util.h"
#include "list.h"

#include <stdint.h>

typedef struct
{
	bedrock_list list;
	size_t size;
} bedrock_memory_pool;

typedef struct
{
	bedrock_node node;
	size_t size;
	unsigned char *memory;
} bedrock_memory_block;

#define BEDROCK_MEMORY_POOL_INIT { LIST_INIT, 0 }

extern uint64_t bedrock_memory;

extern void *bedrock_malloc_pool(bedrock_memory_pool *pool, size_t size);
extern void bedrock_free_pool(bedrock_memory_pool *pool, void *ptr);
extern void *bedrock_realloc_pool(bedrock_memory_pool *pool, void *pointer, size_t size);

extern void *bedrock_malloc(size_t size);
extern void *bedrock_realloc(void *pointer, size_t size);
extern void bedrock_free(void *pointer);
extern char *bedrock_strdup(const char *string);

#endif // BEDROCK_UTIL_MEMORY_H
