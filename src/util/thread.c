#include "server/bedrock.h"
#include "util/thread.h"

#include <errno.h>

bedrock_list thread_list = LIST_INIT;

static void *thread_entry(void *data)
{
	bedrock_thread *thread = data;
	thread->entry(thread->data);
	pthread_exit(0);
}

void bedrock_thread_start(void (*entry)(void *), void *data)
{
	bedrock_thread *thread = bedrock_malloc(sizeof(bedrock_thread));
	int err;

	thread->entry = entry;
	thread->data = data;

	bedrock_list_add(&thread_list, thread);

	err = pthread_create(&thread->handle, NULL, thread_entry, thread);
	if (err)
	{
		bedrock_log(LEVEL_CRIT, "thread: Unable to create thread - %s", strerror(errno));
		bedrock_list_del(&thread_list, thread);
		bedrock_free(thread);
	}
}

void bedrock_thread_process()
{
	bedrock_node *node, *node2;;

	LIST_FOREACH_SAFE(&thread_list, node, node2)
	{
		bedrock_thread *thread = node->data;

		if (thread->exit)
		{
			if (pthread_join(thread->handle, NULL))
				bedrock_log(LEVEL_CRIT, "thread: Unable to join thread - %s", strerror(errno));

			bedrock_free(thread);

			bedrock_list_del_node(&thread_list, node);
			bedrock_free_pool(thread_list.pool, node);
		}
	}
}

