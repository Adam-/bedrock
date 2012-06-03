#include "server/bedrock.h"
#include "util/thread.h"

#include <errno.h>

bedrock_list thread_list = LIST_INIT;

static void *thread_entry(void *data)
{
	bedrock_thread *thread = data;
	thread->entry(thread->data);
	bedrock_assert(sem_post(&thread->exit) == 0, ;);
	pthread_exit(0);
}

void bedrock_thread_start(void (*entry)(void *), void *data)
{
	bedrock_thread *thread = bedrock_malloc(sizeof(bedrock_thread));
	int err;

	if (sem_init(&thread->exit, 0, 0))
	{
		bedrock_log(LEVEL_CRIT, "thread: Unable to create semaphore - %s", strerror(errno));
		bedrock_free(thread);
		return;
	}

	thread->entry = entry;
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
		bedrock_log(LEVEL_DEBUG, "thread: Created thread %d", thread->handle);
}

void bedrock_thread_process()
{
	bedrock_node *node, *node2;;

	LIST_FOREACH_SAFE(&thread_list, node, node2)
	{
		bedrock_thread *thread = node->data;
		int val = 0;

		if (sem_getvalue(&thread->exit, &val) == 0 && val)
		{
			if (pthread_join(thread->handle, NULL))
				bedrock_log(LEVEL_CRIT, "thread: Unable to join thread - %s", strerror(errno));
			else
				bedrock_log(LEVEL_DEBUG, "thread: Joining thread %d", thread->handle);

			sem_destroy(&thread->exit);
			bedrock_free(thread);

			bedrock_list_del_node(&thread_list, node);
			bedrock_free_pool(thread_list.pool, node);
		}
	}
}

