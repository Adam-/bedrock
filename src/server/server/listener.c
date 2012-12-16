#include "server/bedrock.h"
#include "server/client.h"
#include "util/fd.h"
#include "util/io.h"
#include "config/config.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

static struct bedrock_fd fd;

static void accept_client(evutil_socket_t fd, short bedrock_attribute_unused events, void bedrock_attribute_unused *data)
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

	client_fd = accept(fd, &addr.addr, &addrlen);
	if (client_fd < 0)
	{
		bedrock_log(LEVEL_CRIT, "Error accepting client - %s", strerror(errno));
		return;
	}

	evutil_make_socket_nonblocking(client_fd);

	setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

	client = client_create();

	bedrock_fd_open(&client->fd, client_fd, FD_SOCKET, "client fd");
	memcpy(&client->fd.addr, &addr, addrlen);

	io_assign(&client->fd.event_read, client->fd.fd, EV_PERSIST | EV_READ, client_event_read, client);
	io_assign(&client->fd.event_write, client->fd.fd, EV_PERSIST | EV_WRITE, client_event_write, client);

	io_enable(&client->fd.event_read);

	bedrock_log(LEVEL_DEBUG, "Accepted client from %s", client_get_ip(client));
}

void listener_init()
{
	int listen_fd;
	socklen_t opt = 1;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
	{
		bedrock_log(LEVEL_CRIT, "Unable to create socket - %s", strerror(errno));
		abort();
	}

	bedrock_fd_open(&fd, listen_fd, FD_SOCKET, "listen fd");

	evutil_make_socket_nonblocking(fd.fd);

	setsockopt(fd.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (inet_pton(AF_INET, server_ip, &fd.addr.in4.sin_addr) != 1)
	{
		bedrock_log(LEVEL_CRIT, "Unable to call inet_pton - %s", strerror(errno));
		abort();
	}

	fd.addr.in4.sin_family = AF_INET;
	fd.addr.in4.sin_port = htons(server_port);
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

	io_assign(&fd.event_read, fd.fd, EV_PERSIST | EV_READ, accept_client, NULL);
	io_enable(&fd.event_read);
}

void listener_shutdown()
{
	bedrock_fd_close(&fd);
}
