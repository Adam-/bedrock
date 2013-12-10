#include "util/thread.h"
#include "util/memory.h"
#include "util/string.h"

#include <errno.h>

bedrock_list thread_list = LIST_INIT;
bedrock_pipe thread_notify_pipe;

static bedrock_list thread_exited_list = LIST_INIT;
static bedrock_mutex thread_mutex;

static void do_exit_threads(void bedrock_attribute_unused *unused)
{
	bedrock_mutex_lock(&thread_mutex);

	while (thread_exited_list.count)
	{
		struct bedrock_thread *thread = thread_exited_list.head->data;

		bedrock_mutex_unlock(&thread_mutex);
		bedrock_thread_join(thread);
		bedrock_mutex_lock(&thread_mutex);
	}

	bedrock_mutex_unlock(&thread_mutex);
}

void bedrock_threadengine_start()
{
	bedrock_pipe_open(&thread_notify_pipe, "thread exit pipe", do_exit_threads, NULL);
	bedrock_mutex_init(&thread_mutex, "thread mutex");
}

void bedrock_threadengine_stop()
{
	bedrock_pipe_close(&thread_notify_pipe);
	bedrock_mutex_destroy(&thread_mutex);
}

static void *thread_entry(void *data)
{
	struct bedrock_thread *thread = data;
	thread->entry(thread, thread->data);
	bedrock_thread_set_exit(thread);
	pthread_exit(NULL);
	return NULL;
}

struct bedrock_thread *bedrock_thread_start(bedrock_thread_entry entry, bedrock_thread_exit at_exit, void *data)
{
	struct bedrock_thread *thread = bedrock_malloc(sizeof(struct bedrock_thread));
	int err;

	thread->entry = entry;
	thread->at_exit = at_exit;
	thread->data = data;

	bedrock_list_add(&thread_list, thread);

	err = pthread_create(&thread->handle, NULL, thread_entry, thread);
	if (err)
	{
		bedrock_log(LEVEL_CRIT, "thread: Unable to create thread - %s", strerror(errno));
		bedrock_list_del(&thread_list, thread);
		bedrock_free(thread);
		thread = NULL;
	}
	else
		bedrock_log(LEVEL_THREAD, "thread: Created thread %d", (int) thread->handle);

	return thread;
}

bool bedrock_thread_want_exit(struct bedrock_thread *thread)
{
	bool b;
	bedrock_mutex_lock(&thread_mutex);
	b = bedrock_list_has_data(&thread_exited_list, thread);
	bedrock_mutex_unlock(&thread_mutex);
	return b;
}

void bedrock_thread_set_exit(struct bedrock_thread *thread)
{
	bedrock_mutex_lock(&thread_mutex);
	if (bedrock_list_has_data(&thread_exited_list, thread) == false)
	{
		bedrock_list_add(&thread_exited_list, thread);
		bedrock_pipe_notify(&thread_notify_pipe);
	}
	bedrock_mutex_unlock(&thread_mutex);
}

void bedrock_thread_join(struct bedrock_thread *thread)
{
	bedrock_thread_set_exit(thread);

	if (pthread_join(thread->handle, NULL))
		bedrock_log(LEVEL_CRIT, "thread: Unable to join thread - %s", strerror(errno));
	else
		bedrock_log(LEVEL_THREAD, "thread: Joining thread %d", (int) thread->handle);

	if (thread->at_exit)
		thread->at_exit(thread->data);

	bedrock_list_del(&thread_list, thread);
	bedrock_mutex_lock(&thread_mutex);
	bedrock_list_del(&thread_exited_list, thread);
	bedrock_mutex_unlock(&thread_mutex);

	bedrock_free(thread);
}

void bedrock_thread_exit_all()
{
	bedrock_node *node, *node2;;

	LIST_FOREACH_SAFE(&thread_list, node, node2)
	{
		struct bedrock_thread *thread = node->data;
		bedrock_thread_join(thread);
	}
}

void bedrock_mutex_init(bedrock_mutex *mutex, const char *desc)
{
	int i;

	bedrock_strncpy(mutex->desc, desc, sizeof(mutex->desc));

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

void bedrock_cond_init(bedrock_cond *cond, const char *desc)
{
	bedrock_strncpy(cond->desc, desc, sizeof(cond->desc));
	pthread_cond_init(&cond->cond, NULL);
}

void bedrock_cond_destroy(bedrock_cond *cond)
{
	pthread_cond_destroy(&cond->cond);
}

void bedrock_cond_wakeup(bedrock_cond *cond)
{
	pthread_cond_signal(&cond->cond);
}

bool bedrock_cond_wait(bedrock_cond *cond, bedrock_mutex *mutex)
{
	return pthread_cond_wait(&cond->cond, &mutex->mutex) == 0;
}

void bedrock_spinlock_init(bedrock_spinlock *lock, const char *desc)
{
	int i;

	bedrock_strncpy(lock->desc, desc, sizeof(lock->desc));

	/* valgrind does not support spinlocks, so emulate them using mutexes */
	if (running_on_valgrind)
		i = pthread_mutex_init(&lock->u.mutex, NULL);
	else
		i = pthread_spin_init(&lock->u.spinlock, PTHREAD_PROCESS_PRIVATE);
	if (i)
		bedrock_log(LEVEL_CRIT, "thread: Unable to initialize spinlock %s - %s", desc, strerror(errno));
	else
		bedrock_log(LEVEL_THREAD, "thread: Successfully initialized spinlock %s", desc);
}

void bedrock_spinlock_destroy(bedrock_spinlock *lock)
{
	int i;

	bedrock_assert(lock != NULL, return);

	if (running_on_valgrind)
		i = pthread_mutex_destroy(&lock->u.mutex);
	else
		i = pthread_spin_destroy(&lock->u.spinlock);
	if (i)
		bedrock_log(LEVEL_CRIT, "thread: Unable to destroy spinlock %s - %s", lock->desc, strerror(errno));
	else
		bedrock_log(LEVEL_THREAD, "thread: Successfully destroyed spinlock %s", lock->desc);
}

void bedrock_spinlock_lock(bedrock_spinlock *lock)
{
	int i;

	bedrock_assert(lock != NULL, return);

	if (running_on_valgrind)
		i = pthread_mutex_lock(&lock->u.mutex);
	else
		i = pthread_spin_lock(&lock->u.spinlock);
	if (i)
		bedrock_log(LEVEL_CRIT, "thread: Unable to lock spinlock %s - %s", lock->desc, strerror(errno));
	else
		bedrock_log(LEVEL_THREAD, "thread: Successfully locked spinlock %s", lock->desc);
}

void bedrock_spinlock_unlock(bedrock_spinlock *lock)
{
	int i;

	bedrock_assert(lock != NULL, return);

	if (running_on_valgrind)
		i = pthread_mutex_unlock(&lock->u.mutex);
	else
		i = pthread_spin_unlock(&lock->u.spinlock);
	if (i)
		bedrock_log(LEVEL_CRIT, "thread: Unable to unlock spinlock %s - %s", lock->desc, strerror(errno));
	else
		bedrock_log(LEVEL_THREAD, "thread: Successfully unlocked spinlock %s", lock->desc);
}

