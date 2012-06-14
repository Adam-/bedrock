#include "server/bedrock.h"
#include "util/thread.h"
#include "util/memory.h"

#include <errno.h>

bedrock_list thread_list = LIST_INIT;

static void do_join_thread(void *data)
{
	bedrock_thread *thread = data;

	bedrock_thread_set_exit(thread);

	if (pthread_join(thread->handle, NULL))
		bedrock_log(LEVEL_CRIT, "thread: Unable to join thread - %s", strerror(errno));
	else
		bedrock_log(LEVEL_THREAD, "thread: Joining thread %d", thread->handle);

	if (thread->at_exit)
		thread->at_exit(thread->data);

	sem_destroy(&thread->exit);
	bedrock_pipe_close(thread->notify_pipe);
	bedrock_free(thread);

	bedrock_list_del(&thread_list, data);
}

static void *thread_entry(void *data)
{
	bedrock_thread *thread = data;
	thread->entry(thread->data);
	bedrock_thread_set_exit(thread);
	pthread_exit(0);
}

void bedrock_thread_start(bedrock_thread_entry entry, bedrock_thread_exit at_exit, void *data)
{
	bedrock_thread *thread = bedrock_malloc(sizeof(bedrock_thread));
	int err;

	if (sem_init(&thread->exit, 0, 0))
	{
		bedrock_log(LEVEL_CRIT, "thread: Unable to create semaphore - %s", strerror(errno));
		bedrock_free(thread);
		return;
	}

	thread->notify_pipe = bedrock_pipe_open("thread exit pipe", do_join_thread, thread);
	thread->entry = entry;
	thread->at_exit = at_exit;
	thread->data = data;

	bedrock_list_add(&thread_list, thread);

	err = pthread_create(&thread->handle, NULL, thread_entry, thread);
	if (err)
	{
		bedrock_log(LEVEL_CRIT, "thread: Unable to create thread - %s", strerror(errno));
		bedrock_list_del(&thread_list, thread);
		sem_destroy(&thread->exit);
		bedrock_free(thread);
	}
	else
		bedrock_log(LEVEL_THREAD, "thread: Created thread %d", thread->handle);
}

bool bedrock_thread_want_exit(bedrock_thread *thread)
{
	int val = 0;
	return sem_getvalue(&thread->exit, &val) == 0 && val;
}

void bedrock_thread_set_exit(bedrock_thread *thread)
{
	bedrock_assert(sem_post(&thread->exit) == 0, ;);
	bedrock_pipe_notify(thread->notify_pipe);
}

void bedrock_thread_exit_all()
{
	bedrock_node *node, *node2;;

	LIST_FOREACH_SAFE(&thread_list, node, node2)
	{
		bedrock_thread *thread = node->data;
		do_join_thread(thread);
	}
}

void bedrock_mutex_init(bedrock_mutex *mutex, const char *desc)
{
	int i;

	strncpy(mutex->desc, desc, sizeof(mutex->desc));

	i = pthread_mutex_init(&mutex->mutex, NULL);
	if (i)
		bedrock_log(LEVEL_CRIT, "thread: Unable to initialize mutex %s - %s", desc, strerror(errno));
	else
		bedrock_log(LEVEL_THREAD, "thread: Successfully initialized mutex %s", desc);
}

void bedrock_mutex_destroy(bedrock_mutex *mutex)
{
	int i;
	bedrock_assert(mutex != NULL, return);

	i = pthread_mutex_destroy(&mutex->mutex);
	if (i)
		bedrock_log(LEVEL_CRIT, "thread: Unable to destroy mutex %s - %s", mutex->desc, strerror(errno));
	else
		bedrock_log(LEVEL_THREAD, "thread: Successfully destroyed mutex %s", mutex->desc);
}

void bedrock_mutex_lock(bedrock_mutex *mutex)
{
	int i;

	bedrock_assert(mutex != NULL, return);

	i = pthread_mutex_lock(&mutex->mutex);
	if (i)
		bedrock_log(LEVEL_CRIT, "thread: Unable to lock mutex %s - %s", mutex->desc, strerror(errno));
	else
		bedrock_log(LEVEL_THREAD, "thread: Successfully locked mutex %s", mutex->desc);
}

bool bedrock_mutex_trylock(bedrock_mutex *mutex)
{
	bedrock_assert(mutex != NULL, return false);
	return pthread_mutex_trylock(&mutex->mutex) == 0;
}

void bedrock_mutex_unlock(bedrock_mutex *mutex)
{
	int i;

	bedrock_assert(mutex != NULL, return);

	i = pthread_mutex_unlock(&mutex->mutex);
	if (i)
		bedrock_log(LEVEL_CRIT, "thread: Unable to unlock mutex %s - %s", mutex->desc, strerror(errno));
	else
		bedrock_log(LEVEL_THREAD, "thread: Successfully unlocked mutex %s", mutex->desc);
}
