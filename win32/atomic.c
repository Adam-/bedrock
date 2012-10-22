#include <pthread.h>

static pthread_mutex_t mutex;

static inline void init_mutex()
{
	static int i;
	if (i == 0)
	{
		i = 1;
		pthread_mutex_init(&mutex, NULL);
	}
}

long long __sync_fetch_and_add_8(long long *ptr, long long value)
{
	init_mutex();
	pthread_mutex_lock(&mutex);
	*ptr += value;
	pthread_mutex_unlock(&mutex);
	return *ptr;
}

long long __sync_fetch_and_sub_8(long long *ptr, long long value)
{
	init_mutex();
	pthread_mutex_lock(&mutex);
	*ptr -= value;
	pthread_mutex_unlock(&mutex);
	return *ptr;
}

long long __sync_and_and_fetch_8(long long *ptr, long long value)
{
	init_mutex();
	pthread_mutex_lock(&mutex);
	*ptr &= value;
	pthread_mutex_unlock(&mutex);
	return *ptr;
}

long long __sync_or_and_fetch_8(long long *ptr, long long value)
{
	init_mutex();
	pthread_mutex_lock(&mutex);
	*ptr |= value;
	pthread_mutex_unlock(&mutex);
	return *ptr;
}

