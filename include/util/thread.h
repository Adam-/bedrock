#include "util/list.h"

#include <pthread.h>

typedef struct
{
	pthread_t handle;
	bool exit;
	void (*entry)(void *);
	void *data;
} bedrock_thread;

extern bedrock_list thread_list;

extern void bedrock_thread_start(void (*entry)(void *), void *data);
extern void bedrock_thread_process();
