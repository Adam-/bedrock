#include "util/io.h"
#include "server/config/hard.h"

static struct event_base *eb;

#if 0
#include "util/memory.h"

static bedrock_list heap_list;

static void *io_malloc(size_t sz)
{
	void *ptr = bedrock_malloc(sizeof(bedrock_node) + sz);
	bedrock_list_add_node(&heap_list, ptr, NULL);
	return (char *) ptr + sizeof(bedrock_node);
}

static void io_free(void *ptr)
{
	void *orig_ptr = (char *) ptr - sizeof(bedrock_node);
	bedrock_list_del_node(&heap_list, orig_ptr);
	bedrock_free(orig_ptr);
}

static void *io_realloc(void *ptr, size_t sz)
{
	if (ptr == NULL)
		return io_malloc(sz);
	else if (!sz)
	{
		io_free(ptr);
		return NULL;
	}

	void *orig_ptr = (char *) ptr - sizeof(bedrock_node);

	bedrock_list_del_node(&heap_list, orig_ptr);

	ptr = bedrock_realloc(ptr, sizeof(bedrock_node) + sz);

	bedrock_list_add_node(&heap_list, ptr, NULL);

	return (char *) ptr + sizeof(bedrock_node);
}
#endif

void io_init()
{
#if 0
	event_set_mem_functions(io_malloc, io_realloc, io_free);
#endif

#ifdef WIN32
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

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

#ifdef WIN32
	WSACleanup();
#endif
}

void io_assign(struct event *ev, evutil_socket_t fd, short events, event_callback_fn callback, void *callback_arg)
{
	if (event_assign(ev, eb, fd, events, callback, callback_arg) == -1)
		bedrock_log(LEVEL_WARN, "io: error from event_assign()");
}

void io_timer_schedule(struct event *ev, uint64_t ticks_from_now, short events, event_callback_fn callback, void *callback_arg)
{
	struct timeval tv;

	ticks_from_now *= BEDROCK_TICK_LENGTH; // To milliseconds
	ticks_from_now *= 1000; // To microseconds

	tv.tv_sec = ticks_from_now / 1000000;
	tv.tv_usec = ticks_from_now % 1000000;

	if (event_assign(ev, eb, -1, events, callback, callback_arg) == -1)
		bedrock_log(LEVEL_WARN, "io: error from event_assign() scheduling timer");
	else if (event_add(ev, &tv) == -1)
		bedrock_log(LEVEL_WARN, "io: error from event_add() scheduling timer");
}

void io_signal(struct event *ev, int signum, event_callback_fn callback)
{
	if (event_assign(ev, eb, signum, EV_PERSIST | EV_SIGNAL, callback, NULL) == -1)
		bedrock_log(LEVEL_WARN, "io: error from event_assign() installing signal handler");
	else if (event_add(ev, NULL) == -1)
		bedrock_log(LEVEL_WARN, "io: error from event_add() installing signal handler");
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
