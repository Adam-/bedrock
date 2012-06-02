#include "server/bedrock.h"
#include "io/io.h"
#include "server/config.h"

#include <sys/epoll.h>
#include <errno.h>

static bedrock_fd efd;

void io_init()
{
	int fd = epoll_create(1024);
	if (fd < 0)
	{
		bedrock_log(LEVEL_CRIT, "io: Unable to open epoll fd - %s", strerror(errno));
		abort();
	}

	bedrock_fd_open(&efd, fd, false, "epoll file descriptor");
}

void io_shutdown()
{
	bedrock_fd_close(&efd);
}

void io_set(bedrock_fd *fd, bedrock_io_ops add, bedrock_io_ops remove)
{
	unsigned int ops = fd->ops;

	if (remove & OP_READ)
		ops &= ~OP_READ;
	if (remove & OP_WRITE)
		ops &= ~OP_WRITE;

	if (add & OP_READ)
		ops |= OP_READ;
	if (add & OP_WRITE)
		ops |= OP_WRITE;

	if (ops != fd->ops)
	{
		struct epoll_event event;
		int op;

		memset(&event, 0, sizeof(event)); /* To make valgrind shut up */

		event.data.fd = fd->fd;
		event.events = 0;

		if (ops & OP_READ)
			event.events |= EPOLLIN;
		if (ops & OP_WRITE)
			event.events |= EPOLLOUT;

		if (fd->ops == 0)
			op = EPOLL_CTL_ADD;
		else if (event.events == 0)
			op = EPOLL_CTL_DEL;
		else
			op = EPOLL_CTL_MOD;

		if (epoll_ctl(efd.fd, op, fd->fd, &event) != 0)
		{
			bedrock_log(LEVEL_CRIT, "io: Unable to modify epoll event - %s", strerror(errno));
			abort();
		}

		fd->ops = ops;
	}
}

bool io_has(bedrock_fd *fd, bedrock_io_ops flag)
{
	return (fd->ops & flag) != 0;
}

void io_process()
{
	struct epoll_event events[128];
	int num, i;

	num = epoll_wait(efd.fd, events, sizeof(events) / sizeof(struct epoll_event), 1 * 1000);
	bedrock_log(LEVEL_IO_DEBUG, "io: epoll returned %d results", num);

	if (num < 0)
	{
		if (errno != EINTR)
			bedrock_log(LEVEL_CRIT, "io: Unable to epoll wait - %s", strerror(errno));
		return;
	}

	bedrock_update_time();

	for (i = 0; i < num; ++i)
	{
		struct epoll_event *ev = &events[i];
		bedrock_fd *fd = bedrock_fd_find(ev->data.fd);
		bedrock_assert(fd != NULL, continue);

		if (ev->events & (EPOLLIN | EPOLLHUP | EPOLLERR))
		{
			if (fd->read_handler != NULL)
				fd->read_handler(fd, fd->read_data);
		}
		if (ev->events & (EPOLLOUT | EPOLLHUP | EPOLLERR))
		{
			if (fd->write_handler != NULL)
				fd->write_handler(fd, fd->write_data);
		}
	}
}
