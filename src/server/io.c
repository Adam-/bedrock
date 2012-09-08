#include "server/bedrock.h"
#include "server/io.h"
#include "config/hard.h"

static struct event_base *eb;

void io_init()
{
	eb = event_base_new();
	if (eb == NULL)
	{
		bedrock_log(LEVEL_CRIT, "io: Unable to initialize i/o subsystem");
		abort();
	}
}

void io_shutdown()
{
	event_base_free(eb);
	eb = NULL;
}

void io_assign(struct event *ev, evutil_socket_t fd, short events, event_callback_fn callback, void *callback_arg)
{
	if (event_assign(ev, eb, fd, events, callback, callback_arg) == -1)
		bedrock_log(LEVEL_WARN, "io: error from event_assign()");
}

void io_timer_schedule(uint64_t ticks_from_now, short events, event_callback_fn callback, void *callback_arg)
{
	struct event *ev;
	struct timeval tv;

	ticks_from_now *= BEDROCK_TICK_LENGTH; // To milliseconds
	ticks_from_now *= 1000; // To microseconds

	tv.tv_sec = ticks_from_now / 1000000;
	tv.tv_usec = ticks_from_now % 1000000;

	ev = event_new(eb, -1, events, callback, callback_arg);
	if (event_add(ev, &tv) == -1)
	{
		bedrock_log(LEVEL_WARN, "io: error from event_add() scheduling timer");
		event_free(ev);
	}
}

void io_enable(struct event *ev)
{
	if (event_add(ev, NULL) == -1)
		bedrock_log(LEVEL_WARN, "io: error from event_add()");
}

void io_disable(struct event *ev)
{
	if (event_del(ev) == -1)
		bedrock_log(LEVEL_WARN, "io: error from event_del()");
}

bool io_is_pending(struct event *ev, short events)
{
	return event_pending(ev, events, NULL) != 0;
}

void io_process()
{
	if (event_base_loop(eb, EVLOOP_ONCE) == -1)
		bedrock_log(LEVEL_WARN, "io: error from event_base_loop()");
}
