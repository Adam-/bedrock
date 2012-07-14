#ifndef BEDROCK_UTIL_FD_H
#define BEDROCK_UTIL_FD_H

#include "util/util.h"

#include <netinet/in.h>

typedef enum
{
	FD_ENGINE,
	FD_FILE,
	FD_SOCKET,
	FD_PIPE
} bedrock_fd_type;

struct bedrock_fd
{
	int fd;
	bedrock_fd_type type;
	char desc[32];
	unsigned int ops;
	bool open;

	void (*read_handler)(struct bedrock_fd *, void *data);
	void *read_data;
	void (*write_handler)(struct bedrock_fd *, void *data);
	void *write_data;

	union
	{
		struct sockaddr in;
		struct sockaddr_in in4;
		struct sockaddr_in6 in6;
	} addr;
};

#include "util/thread.h"

extern bedrock_mutex fdlist_mutex;
extern bedrock_list fdlist;

extern void bedrock_fd_open(struct bedrock_fd *f, int fd, bedrock_fd_type type, const char *desc);
extern void bedrock_fd_close(struct bedrock_fd *f);
extern struct bedrock_fd *bedrock_fd_find(int fd);

#endif // BEDROCK_UTIL_FD_H
