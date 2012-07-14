#include "server/bedrock.h"
#include "io/io.h"
#include "util/memory.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

static void pipe_reader(struct bedrock_fd *fd, void *data)
{
	bedrock_pipe *p = data;
	char buf[32];

	while (read(fd->fd, buf, sizeof(buf)) == sizeof(buf));

	p->on_notify(p->data);
}

void bedrock_pipe_open(bedrock_pipe *p, const char *desc, void (*on_notify)(void *), void *data)
{
	int fds[2];
	char fulldesc[32];

	if (pipe(fds))
	{
		bedrock_log(LEVEL_CRIT, "pipe: Unable to create pipe - %s", strerror(errno));
		return;
	}

	fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL, 0) | O_NONBLOCK);
	fcntl(fds[1], F_SETFL, fcntl(fds[1], F_GETFL, 0) | O_NONBLOCK);

	snprintf(fulldesc, sizeof(fulldesc), "read pipe - %s", desc);
	bedrock_fd_open(&p->read_fd, fds[0], FD_PIPE, fulldesc);

	snprintf(fulldesc, sizeof(fulldesc), "write pipe - %s", desc);
	bedrock_fd_open(&p->write_fd, fds[1], FD_PIPE, fulldesc);

	p->on_notify = on_notify;
	p->data = data;

	p->read_fd.read_handler = pipe_reader;
	p->read_fd.read_data = p;

	io_set(&p->read_fd, OP_READ, 0);
}

void bedrock_pipe_close(bedrock_pipe *p)
{
	io_set(&p->read_fd, 0, ~0);
	bedrock_fd_close(&p->read_fd);
	bedrock_fd_close(&p->write_fd);
}

void bedrock_pipe_notify(bedrock_pipe *p)
{
	char dummy = '*';
	bedrock_assert(write(p->write_fd.fd, &dummy, 1) == 1, ;);
}
