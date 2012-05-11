#ifndef BEDROCK_UTIL_H
#define BEDROCK_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define DEBUG

typedef enum bool_ { false, true } bool;

#ifndef DEBUG
# define bedrock_assert(var)
# define bedrock_assert_ret(var, ret)
#else
# define bedrock_assert(var) \
	if (!(var)) \
	{ \
		printf("Debug assertion failed: %s:%d\n", __FILE__, __LINE__); \
		return; \
	}
# define bedrock_assert_ret(var, ret) \
	if (!(var)) \
	{ \
		printf("Debug assertion failed: %s:%d\n", __FILE__, __LINE__); \
		return (ret); \
	}
#endif

#endif // BEDROCK_UTIL_H
