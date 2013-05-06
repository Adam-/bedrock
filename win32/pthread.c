#include "pthread.h"
#include "util/memory.h"

struct thread_info
{
	void *(*entry)(void *);
	void *param;
};

static DWORD WINAPI entry_point(void *parameter)
{
	struct thread_info *ti = (struct thread_info *) parameter;
	ti->entry(ti->param);
	free(ti);
	return 0;
}

int pthread_attr_init(pthread_attr_t *attr)
{
	/* No need for this */
	return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int state)
{
	/* No need for this */
	return 0;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*entry)(void *), void *param)
{
	struct thread_info *ti = malloc(sizeof(struct thread_info));
	ti->entry = entry;
	ti->param = param;

	*thread = CreateThread(NULL, 0, entry_point, ti, 0, NULL);
	if (!*thread)
	{
		free(ti);
		return -1;
	}

	return 0;
}

int pthread_join(pthread_t thread, void **value)
{
	if (WaitForSingleObject(thread, INFINITE) == WAIT_FAILED)
		return -1;
	CloseHandle(thread);
	return 0;
}

void pthread_exit(int i)
{
	ExitThread(i);
}

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	InitializeCriticalSection(mutex);
	return 0;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	DeleteCriticalSection(mutex);
	return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	EnterCriticalSection(mutex);
	return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	return !TryEnterCriticalSection(mutex);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	LeaveCriticalSection(mutex);
	return 0;
}

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
	*cond = CreateEvent(NULL, false, false, NULL);
	if (*cond == NULL)
		return -1;
	return 0;
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
	return !CloseHandle(*cond);
}

int pthread_cond_signal(pthread_cond_t *cond)
{
	return !PulseEvent(*cond);
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	LeaveCriticalSection(mutex);
	WaitForSingleObject(*cond, INFINITE);
	EnterCriticalSection(mutex);
	return 0;
}
