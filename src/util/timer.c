#include "util/timer.h"
#include "util/list.h"
#include "util/memory.h"
#include "server/config.h"
#include "server/bedrock.h"

static bedrock_list timer_list;

static void insert_timer(bedrock_timer *timer)
{
	bedrock_node *node;

	LIST_FOREACH(&timer_list, node)
	{
		bedrock_timer *t = node->data;

		if (timer->tick.tv_sec > t->tick.tv_sec)
			continue;
		else if (timer->tick.tv_sec == t->tick.tv_sec && timer->tick.tv_nsec > t->tick.tv_nsec)
			continue;

		bedrock_list_add_node_before(&timer_list, bedrock_malloc(sizeof(bedrock_node)), node, timer);
		return;
	}

	bedrock_list_add(&timer_list, timer);
}

void bedrock_timer_schedule(uint64_t ticks_from_now, void (*func)(void *), void *data)
{
	bedrock_timer *timer = bedrock_malloc(sizeof(bedrock_timer));

	ticks_from_now *= BEDROCK_TICK_LENGTH; // To milliseconds
	ticks_from_now *= 1000000; // To nanoseconds

	timer->tick = bedrock_time;
	timer->tick.tv_sec += ticks_from_now / 1000000000;

	ticks_from_now %= 1000000000;
	ticks_from_now += timer->tick.tv_nsec;
	timer->tick.tv_sec += ticks_from_now / 1000000000;
	timer->tick.tv_nsec = ticks_from_now % 1000000000;

	timer->data = data;
	timer->func = func;

	bedrock_log(LEVEL_DEBUG, "timer: New timer %p scheduled for %ld.%ld, now it is %ld.%ld", timer, timer->tick.tv_sec, timer->tick.tv_nsec, bedrock_time.tv_sec, bedrock_time.tv_nsec);

	insert_timer(timer);
}

void bedrock_timer_process()
{
	bedrock_node *node, *node2;

	LIST_FOREACH_SAFE(&timer_list, node, node2)
	{
		bedrock_timer *t = node->data;

		if (t->tick.tv_sec > bedrock_time.tv_sec)
			break;
		else if (t->tick.tv_sec == bedrock_time.tv_sec && t->tick.tv_nsec > bedrock_time.tv_nsec)
			break;

		bedrock_log(LEVEL_DEBUG, "timer: Calling timer %p, scheduled for %ld.%ld", t, t->tick.tv_sec, t->tick.tv_nsec);

		t->func(t->data);

		bedrock_list_del_node(&timer_list, node);
		bedrock_free(t);
		bedrock_free(node);
	}
}

