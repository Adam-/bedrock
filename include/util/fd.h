#ifndef BEDROCK_UTIL_FD_H
#define BEDROCK_UTIL_FD_H

#include "util/util.h"

#include <event2/event.h>
#include <event2/event_struct.h>

#include <netinet/in.h>
#include <sys/un.h>

typedef enum
{
	FD_FILE,
	FD_SOCKET,
	FD_PIPE
} bedrock_fd_type;

struct bedrock_fd
{
	int fd;
	bedrock_fd_type type;
	char desc[32];
	bool open;

	/* Events used for sockets and pipes */
	struct event event_read;
	struct event event_write;

	union
	{
		struct sockaddr in;
		struct sockaddr_in in4;
		struct sockaddr_in6 in6;
		struct sockaddr_un un;
	} addr;
};

#include "util/thread.h"

extern bedrock_mutex fdlist_mutex;
extern bedrock_list fdlist;

extern void bedrock_fd_open(struct bedrock_fd *f, int fd, bedrock_fd_type type, const char *desc);
extern void bedrock_fd_close(struct bedrock_fd *f);
extern struct bedrock_fd *bedrock_fd_find(int fd);

#endif // BEDROCK_UTIL_FD_H
