#include "config.h"
#include "util/memory.h"

int running_on_valgrind = 0;

#ifdef HAVE_VALGRIND_H
# include <valgrind/valgrind.h>
#endif

void util_init()
{
#ifdef RUNNING_ON_VALGRIND
	running_on_valgrind = RUNNING_ON_VALGRIND;
#endif
	bedrock_memory_init();
	bedrock_fd_init();
}

