#ifndef BEDROCK_UTIL_MEMORY_H
#define BEDROCK_UTIL_MEMORY_H

#include "util.h"
#include "thread.h"

extern long long memory_size;
extern bedrock_spinlock memory_lock;

extern void *bedrock_malloc(size_t size);
extern void *bedrock_realloc(void *pointer, size_t size);
extern void bedrock_free(void *pointer);

#endif // BEDROCK_UTIL_MEMORY_H
