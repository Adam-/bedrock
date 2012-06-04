#include <time.h>
#include <stdint.h>

typedef void (*timer_callback)(void *);

typedef struct
{
	struct timespec tick;
	void *data;
	timer_callback callback;
	void (*func)(void *);
} bedrock_timer;

extern void bedrock_timer_schedule(uint64_t ticks_from_now, timer_callback callback, void *data);
extern void bedrock_timer_cancel_all_for(timer_callback callback);
extern void bedrock_timer_process();
