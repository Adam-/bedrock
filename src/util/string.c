#include "util/string.h"

/** Copy a string
 * @param string String to copy
 * @return A copy of the string
 */
char *bedrock_strdup(const char *string)
{
	char *memory;

	bedrock_assert_ret(string != NULL, NULL);

	memory = bedrock_malloc(strlen(string) + 1);
	strcpy(memory, string);
	return memory;
}
