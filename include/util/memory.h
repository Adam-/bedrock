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

extern uint64_t bedrock_memory;

extern void *bedrock_malloc_pool(bedrock_memory_pool *pool, size_t size);
extern void bedrock_free_pool(bedrock_memory_pool *pool, void *ptr);

extern void *bedrock_malloc(size_t size);
extern void *bedrock_realloc(void *pointer, size_t size);
extern void bedrock_free(void *pointer);
extern char *bedrock_strdup(const char *string);

