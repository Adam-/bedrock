#include "server/bedrock.h"
#include "io/io.h"

#include <sys/epoll.h>
#include <errno.h>

static bedrock_fd efd;

void bedrock_io_init()
{
	int fd = epoll_create(1024);
	if (fd < 0)
	{
		bedrock_log(LEVEL_CRIT, "Unable to open epoll fd - %s", strerror(errno));
		abort();
	}

	bedrock_fd_open(&efd, fd, false, "epoll file descriptor");
}

void bedrock_io_shutdown()
{
	bedrock_fd_close(&efd);
}

void bedrock_io_set(bedrock_fd *fd, bedrock_io_ops add, bedrock_io_ops remove)
{
	struct epoll_event event;

	event.data.fd = fd->fd;
	event.events = fd->ops;

	if (remove & OP_READ)
		event.events &= ~EPOLLIN;
	 if (remove & OP_WRITE)
		event.events &= ~EPOLLOUT;

	if (add & OP_READ)
		event.events |= EPOLLIN;
	if (add & OP_WRITE)
		event.events |= EPOLLOUT;

	if (event.events != fd->ops)
	{
		int op;

		if (fd->ops == 0)
			op = EPOLL_CTL_ADD;
		else if (event.events == 0)
			op = EPOLL_CTL_DEL;
		else
			op = EPOLL_CTL_MOD;

		if (epoll_ctl(efd.fd, op, fd->fd, &event) != 0)
		{
			bedrock_log(LEVEL_CRIT, "Unable to modify epoll event - %s", strerror(errno));
			abort();
		}

		fd->ops = event.events;
	}
}

void bedrock_io_process()
{
	struct epoll_event events[128];
	int num, i;

	num = epoll_wait(efd.fd, events, sizeof(events) / sizeof(struct epoll_event), 1);

	if (num < 0)
	{
		if (errno != EINTR)
			bedrock_log(LEVEL_CRIT, "Unable to epoll wait - %s", strerror(errno));
		return;
	}

	for (i = 0; i < num; ++i)
	{
		struct epoll_event *ev = &events[i];
		bedrock_fd *fd = bedrock_fd_find(ev->data.fd);
		bedrock_assert(fd != NULL);

		if (ev->events & (EPOLLIN | EPOLLHUP | EPOLLERR))
		{
			if (fd->read_handler != NULL)
				fd->read_handler(fd);
		}
		if (ev->events & (EPOLLOUT | EPOLLHUP | EPOLLERR))
		{
			if (fd->write_handler != NULL)
				fd->write_handler(fd);
		}
	}
}
