#ifndef SERVER_BEDROCK_H
#define SERVER_BEDROCK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include "util/util.h"

#define BEDROCK_VERSION 0x0001

#define BEDROCK_VERSION_MAJOR 0
#define BEDROCK_VERSION_MINOR 1
#define BEDROCK_VERSION_EXTRA "-beta"

extern bool bedrock_running;
extern time_t bedrock_start;
extern struct timespec bedrock_time;
extern uint16_t bedrock_tick;

typedef enum
{
	LEVEL_CRIT,
	LEVEL_WARN,
	LEVEL_INFO,
	LEVEL_DEBUG,
	LEVEL_COLUMN,
	LEVEL_NBT_DEBUG,
	LEVEL_BUFFER,
	LEVEL_IO_DEBUG,
	LEVEL_PACKET_DEBUG
} bedrock_log_level;

extern void bedrock_update_time();
extern void bedrock_log(bedrock_log_level level, const char *msg, ...);

#endif // SERVER_BEDROCK_H
