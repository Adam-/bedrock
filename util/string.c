#include "util/string.h"
#include "util/memory.h"

/** Copy a string
 * @param string String to copy
 * @return A copy of the string
 */
char *bedrock_strdup(const char *string)
{
	char *memory;

	bedrock_assert(string != NULL, return NULL);

	memory = bedrock_malloc(strlen(string) + 1);
	strcpy(memory, string);
	return memory;
}

/** Appends src to dest while not overflowing dest
 * @param dest A null terminated string to append src to
 * @param src A null terminated string to append to dest
 */
void bedrock_strlcat(char *dest, const char *src, size_t dest_size)
{
	size_t dest_len = strlen(dest);

	for (; dest_len < dest_size - 1 && *src; ++dest_len)
		dest[dest_len] = *src++;
	dest[dest_len] = 0;
}

/** Like strncpy, but always nul terminates the buffer
 */
void bedrock_strncpy(char *dest, const char *src, size_t sz)
{
	strncpy(dest, src, sz);
	if (sz)
		dest[sz - 1] = 0;
}

