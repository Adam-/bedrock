#include <time.h>
#include <stdint.h>

typedef struct
{
	struct timespec tick;
	void *data;
	void (*func)(void *);
} bedrock_timer;

extern void bedrock_timer_schedule(uint64_t ticks_from_now, void (*func)(void *), void *data);

extern void bedrock_timer_process();
