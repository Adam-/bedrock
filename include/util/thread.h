#ifndef BEDROCK_UTIL_THREAD_H
#define BEDROCK_UTIL_THREAD_H

#include "util/list.h"
#include "util/pipe.h"

#include <pthread.h>
#include <semaphore.h>

typedef void (*bedrock_thread_entry)(void *);
typedef void (*bedrock_thread_exit)(void *);

typedef struct
{
	pthread_t handle;
	sem_t exit;
	bedrock_pipe *notify_pipe;
	bedrock_thread_entry entry;
	bedrock_thread_exit at_exit;
	void *data;
} bedrock_thread;

typedef struct
{
	pthread_mutex_t mutex;
	char desc[32];
} bedrock_mutex;

#define BEDROCK_MUTEX_INIT(desc) { PTHREAD_MUTEX_INITIALIZER, desc }

extern bedrock_list thread_list;

extern void bedrock_thread_start(bedrock_thread_entry entry, bedrock_thread_exit at_exit, void *data);
extern bool bedrock_thread_want_exit(bedrock_thread *thread);
extern void bedrock_thread_set_exit(bedrock_thread *thread);
extern void bedrock_thread_exit_all();

extern void bedrock_mutex_init(bedrock_mutex *mutex, const char *desc);
extern void bedrock_mutex_destroy(bedrock_mutex *mutex);
extern void bedrock_mutex_lock(bedrock_mutex *mutex);
extern bool bedrock_mutex_trylock(bedrock_mutex *mutex);
extern void bedrock_mutex_unlock(bedrock_mutex *mutex);

#endif // BEDROCK_UTIL_THREAD_H
