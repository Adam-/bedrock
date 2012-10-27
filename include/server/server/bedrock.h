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

extern uint16_t bedrock_conf_log_level;

#endif // SERVER_BEDROCK_H
