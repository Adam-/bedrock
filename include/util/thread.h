#ifndef BEDROCK_UTIL_THREAD_H
#define BEDROCK_UTIL_THREAD_H

#include <pthread.h>

struct bedrock_mutex
{
	pthread_mutex_t mutex;
	char desc[32];
};
typedef struct bedrock_mutex bedrock_mutex;

struct bedrock_cond
{
	pthread_cond_t cond;
	char desc[32];
};
typedef struct bedrock_cond bedrock_cond;

struct bedrock_spinlock
{
	pthread_spinlock_t spinlock;
	char desc[32];
};
typedef struct bedrock_spinlock bedrock_spinlock;

#include "util/list.h"
#include "util/pipe.h"

struct bedrock_thread;

typedef void (*bedrock_thread_entry)(struct bedrock_thread *, void *);
typedef void (*bedrock_thread_exit)(void *);

struct bedrock_thread
{
	pthread_t handle;
	bedrock_thread_entry entry;
	bedrock_thread_exit at_exit;
	void *data;
};

extern bedrock_list thread_list;
extern bedrock_pipe thread_notify_pipe;

extern void bedrock_threadengine_start();
extern void bedrock_threadengine_stop();

extern struct bedrock_thread *bedrock_thread_start(bedrock_thread_entry entry, bedrock_thread_exit at_exit, void *data);
extern bool bedrock_thread_want_exit(struct bedrock_thread *thread);
extern void bedrock_thread_set_exit(struct bedrock_thread *thread);
extern void bedrock_thread_join(struct bedrock_thread *thread);
extern void bedrock_thread_exit_all();

extern void bedrock_mutex_init(bedrock_mutex *mutex, const char *desc);
extern void bedrock_mutex_destroy(bedrock_mutex *mutex);
extern void bedrock_mutex_lock(bedrock_mutex *mutex);
extern bool bedrock_mutex_trylock(bedrock_mutex *mutex);
extern void bedrock_mutex_unlock(bedrock_mutex *mutex);

extern void bedrock_cond_init(bedrock_cond *cond, const char *desc);
extern void bedrock_cond_destroy(bedrock_cond *cond);
extern void bedrock_cond_wakeup(bedrock_cond *cond);
extern bool bedrock_cond_wait(bedrock_cond *cond, bedrock_mutex *mutex);

extern void bedrock_spinlock_init(bedrock_spinlock *lock, const char *desc);
extern void bedrock_spinlock_destroy(bedrock_spinlock *lock);
extern void bedrock_spinlock_lock(bedrock_spinlock *lock);
extern void bedrock_spinlock_unlock(bedrock_spinlock *lock);

#endif // BEDROCK_UTIL_THREAD_H
