#include "util/memory.h"
#include "util/file.h"

#include <sys/types.h>
#include <sys/stat.h>

/* Read everything from 'fd' into memory and return it */
unsigned char *bedrock_file_read(int fd, size_t *file_size)
{
	struct stat file_info;
	unsigned char *contents;

	*file_size = 0;

	if (fstat(fd, &file_info) != 0)
		return NULL;

	contents = bedrock_malloc(file_info.st_size);

	if (bedrock_file_read_buf(fd, contents, file_info.st_size) == false)
	{
		bedrock_free(contents);
		contents = NULL;
	}
	else
		*file_size = file_info.st_size;

	return contents;
}

bool bedrock_file_read_buf(int fd, void *dest, size_t want)
{
	int i;
	size_t used = 0;

	while ((i = read(fd, (char *) dest + used, want - used)) > 0)
		used += i;

	return i == 0;
}

