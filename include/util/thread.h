#include "util/list.h"

#include <pthread.h>
#include <semaphore.h>

typedef struct
{
	pthread_t handle;
	volatile sem_t exit;
	void (*entry)(void *);
	void *data;
} bedrock_thread;

extern bedrock_list thread_list;

extern void bedrock_thread_start(void (*entry)(void *), void *data);
extern void bedrock_thread_process();
