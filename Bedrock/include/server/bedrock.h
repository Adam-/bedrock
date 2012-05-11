#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include "util/util.h"

typedef enum
{
	LEVEL_CRIT,
	LEVEL_WARN,
	LEVEL_INFO,
	LEVEL_DEBUG
} bedrock_log_level;

extern void bedrock_log(bedrock_log_level level, const char *msg, ...);

bool bedrock_running;
