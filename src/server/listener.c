#include "server/bedrock.h"
#include "server/config.h"
#include "io/io.h"
#include "server/client.h"
#include "util/fd.h"

#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

static void accept_client(bedrock_fd *fd, void *unused)
{
	struct bedrock_client *client;
	int client_fd;
	union
	{
		struct sockaddr addr;
		struct sockaddr_in addr4;
		struct sockaddr_in6 addr6;
	} addr;
	socklen_t addrlen = sizeof(addr);
	socklen_t opt = 1;

	client_fd = accept(fd->fd, &addr.addr, &addrlen);
	if (client_fd < 0)
	{
		bedrock_log(LEVEL_CRIT, "Error accepting client - %s", strerror(errno));
		return;
	}

	fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK);

	setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

	client = client_create();
	memcpy(&client->fd.addr, &addr, addrlen);

	bedrock_log(LEVEL_DEBUG, "Accepted client from %s", client_get_ip(client));

	bedrock_fd_open(&client->fd, client_fd, FD_SOCKET, "client fd");

	client->fd.read_handler = client_event_read;
	client->fd.write_handler = client_event_write;

	client->fd.read_data = client;
	client->fd.write_data = client;

	io_set(&client->fd, OP_READ, 0);
}

void listener_init()
{
	static bedrock_fd fd;
	int listen_fd;
	socklen_t opt = 1;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
	{
		bedrock_log(LEVEL_CRIT, "Unable to create socket - %s", strerror(errno));
		abort();
	}

	bedrock_fd_open(&fd, listen_fd, FD_SOCKET, "listen fd");

	fcntl(fd.fd, F_SETFL, fcntl(fd.fd, F_GETFL, 0) | O_NONBLOCK);

	setsockopt(fd.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (inet_pton(AF_INET, BEDROCK_LISTEN_IP, &fd.addr.in4.sin_addr) != 1)
	{
		bedrock_log(LEVEL_CRIT, "Unable to call inet_pton - %s", strerror(errno));
		abort();
	}

	fd.addr.in4.sin_family = AF_INET;
	fd.addr.in4.sin_port = htons(BEDROCK_LISTEN_PORT);
	if (bind(fd.fd, &fd.addr.in, sizeof(fd.addr.in4)) < 0)
	{
		bedrock_log(LEVEL_CRIT, "Unable to bind - %s", strerror(errno));
		abort();
	}

	if (listen(fd.fd, SOMAXCONN) < 0)
	{
		bedrock_log(LEVEL_CRIT, "Unable to listen - %s", strerror(errno));
		abort();
	}

	fd.read_handler = accept_client;

	bedrock_fd_open(&fd, fd.fd, FD_SOCKET, "listener");
	io_set(&fd, OP_READ, 0);
}
