
struct timespec
{
	long tv_sec;
	long tv_nsec;
};

extern struct tm *localtime_r(const time_t *timep, struct tm *result);

typedef enum { CLOCK_MONOTONIC } clockid_t;
extern int clock_gettime(clockid_t clk_id, struct timespec *tp);

