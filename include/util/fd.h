#ifndef BEDROCK_UTIL_FD_H
#define BEDROCK_UTIL_FD_H

#include "util.h"
#include "list.h"

#include <netinet/in.h>

typedef enum
{
	FD_ENGINE,
	FD_FILE,
	FD_SOCKET,
	FD_PIPE
} bedrock_fd_type;

typedef struct _bedrock_fd
{
	int fd;
	bedrock_fd_type type;
	char desc[32];
	unsigned int ops;
	bool open;

	void (*read_handler)(struct _bedrock_fd *, void *data);
	void *read_data;
	void (*write_handler)(struct _bedrock_fd *, void *data);
	void *write_data;

	union
	{
		struct sockaddr in;
		struct sockaddr_in in4;
		struct sockaddr_in6 in6;
	} addr;
} bedrock_fd;

bedrock_list fdlist;

extern void bedrock_fd_open(bedrock_fd *f, int fd, bedrock_fd_type type, const char *desc);
extern void bedrock_fd_close(bedrock_fd *f);
extern bedrock_fd *bedrock_fd_find(int fd);

#endif // BEDROCK_UTIL_FD_H
