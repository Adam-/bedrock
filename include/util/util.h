#ifndef BEDROCK_UTIL_H
#define BEDROCK_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>

typedef enum bool_ { false, true } bool;

#define bedrock_assert(var, what) \
	if (!(var)) \
	{ \
		printf("Debug assertion failed: %s:%d\n", __FILE__, __LINE__); \
		what; \
	}

// Ignore gcc warnings about unused variables
#if defined(__GNUC__) || defined(__clang__) || defined(__ICC)
# define bedrock_attribute_unused __attribute__((unused))
#else
# define bedrock_attribute_unused
#endif

#endif // BEDROCK_UTIL_H
