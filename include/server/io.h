#include "util/fd.h"

extern void io_init();
extern void io_shutdown();
extern void io_assign(struct event *ev, evutil_socket_t fd, short events, event_callback_fn callback, void *callback_arg);
extern void io_timer_schedule(struct event *ev, uint64_t ticks_from_now, short events, event_callback_fn callback, void *callback_arg);
extern void io_enable(struct event *ev);
extern void io_disable(struct event *ev);
extern bool io_is_pending(struct event *ev, short events);
extern void io_process();
