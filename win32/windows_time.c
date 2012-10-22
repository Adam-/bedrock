#include "util/util.h"
#include "windows_time.h"

#include <time.h>

struct tm *localtime_r(const time_t *timep, struct tm *result)
{
	*result = *localtime(timep);
	return result;
}

int clock_gettime(clockid_t bedrock_attribute_unused clk_id, struct timespec *tp)
{
	clock_t t = clock();
	if (t == -1)
		return -1;

	tp->tv_sec = t / 1000;
	tp->tv_nsec = (t % 1000) * 1000000;

	return 0;
}

