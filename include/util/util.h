#ifndef BEDROCK_UTIL_H
#define BEDROCK_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#ifndef WIN32
# include <unistd.h>
#else
# define _WINSOCKAPI_
# include <windows.h>
# include <Ws2tcpip.h>
# define PATH_MAX MAX_PATH
# define snprintf _snprintf
# define strcasecmp stricmp
# define write _write
# define lseek _lseek
# define inline
#endif

#define bedrock_assert(var, what) \
	if (!(var)) \
	{ \
		printf("Debug assertion failed: %s:%d\n", __FILE__, __LINE__); \
		what; \
	}

// Ignore gcc warnings about unused variables
#if defined(__GNUC__) || defined(__clang__) || defined(__ICC)
# define bedrock_attribute_unused __attribute__((unused))
# define bedrock_printf(last) __attribute__((format(printf, last, last + 1)))
#else
# define bedrock_attribute_unused
# define bedrock_printf(last)
#endif

enum bedrock_log_level
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
};
typedef enum bedrock_log_level bedrock_log_level;

extern void bedrock_log(bedrock_log_level level, const char *msg, ...) bedrock_printf(2);

extern int running_on_valgrind;

extern void util_init();

#endif // BEDROCK_UTIL_H
