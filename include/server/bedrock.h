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
extern uint32_t entity_id;

typedef enum
{
	LEVEL_CRIT         = 1 << 1,
	LEVEL_WARN         = 1 << 2,
	LEVEL_INFO         = 1 << 3,
	LEVEL_DEBUG        = 1 << 4,
	LEVEL_COLUMN       = 1 << 5,
	LEVEL_NBT_DEBUG    = 1 << 6,
	LEVEL_THREAD       = 1 << 7,
	LEVEL_BUFFER       = 1 << 8,
	LEVEL_IO_DEBUG     = 1 << 9,
	LEVEL_PACKET_DEBUG = 1 << 10
} bedrock_log_level;

extern uint16_t bedrock_conf_log_level;

extern void bedrock_update_time();
extern void bedrock_log(bedrock_log_level level, const char *msg, ...);

#endif // SERVER_BEDROCK_H
