#include "util/util.h"
#include "console/console.h"
#include "console/ncurses.h"

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>

static int socket_fd;
static int stdin_fd;

#define SOCKET_NAME "./bedrock.console.socket"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static void socket_read()
{
	char buf[512];
	int i = recv(socket_fd, buf, sizeof(buf) - 1, 0);
	if (i <= 0)
	{
		ncurses_print("\nLost connection to server\n");
		sleep(2);
		console_running = false;
	}
	else
	{
		buf[i] = 0;
		ncurses_print(buf);
	}
}

bool socket_init()
{
	union
	{
		struct sockaddr sa;
		struct sockaddr_un un;
	} addr;

	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (socket_fd < 0)
	{
		fprintf(stderr, "Unable to create socket - %s\n", strerror(errno));
		return false;
	}

	addr.un.sun_family = AF_UNIX;
	strncpy(addr.un.sun_path, SOCKET_NAME, sizeof(addr.un.sun_path));

	if (connect(socket_fd, &addr.sa, sizeof(addr)) < 0)
	{
		fprintf(stderr, "Unable to connect - %s\n", strerror(errno));
		return false;
	}
	
	stdin_fd = fileno(stdin);

	return true;
}

void socket_shutdown()
{
	close(socket_fd);
}

void socket_process()
{
	fd_set fds;
	int i;

	FD_ZERO(&fds);

	FD_SET(socket_fd, &fds);
	FD_SET(stdin_fd, &fds);

	i = select(MAX(socket_fd, stdin_fd) + 1, &fds, NULL, NULL, NULL);
	if (i < 0)
	{
		sleep(1);
		return;
	}

	if (FD_ISSET(socket_fd, &fds))
		socket_read();
	if (FD_ISSET(stdin_fd, &fds))
		ncurses_read();
}

void socket_send(const char *data)
{
	int i = send(socket_fd, data, strlen(data), 0);
	if (i == -1)
	{
		ncurses_print("\nLost connection to server\n");
		sleep(2);
		console_running = false;
	}
}

