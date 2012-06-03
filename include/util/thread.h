#include "util/list.h"

#include <pthread.h>
#include <semaphore.h>

typedef struct
{
	pthread_t handle;
	sem_t exit;
	void (*entry)(void *);
	void *data;
} bedrock_thread;


typedef struct
{
	pthread_mutex_t mutex;
	char desc[32];
} bedrock_mutex;

extern bedrock_list thread_list;

extern void bedrock_thread_start(void (*entry)(void *), void *data);
extern void bedrock_thread_process();

extern bedrock_mutex *bedrock_mutex_init(const char *desc);
extern void bedrock_mutex_destroy(bedrock_mutex *mutex);
extern void bedrock_mutex_lock(bedrock_mutex *mutex);
extern bool bedrock_mutex_trylock(bedrock_mutex *mutex);
extern void bedrock_mutex_unock(bedrock_mutex *mutex);
