#include "server/bedrock.h"
#include "io/io.h"
#include "server/client.h"

#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

static void accept_client(bedrock_fd *fd)
{
	bedrock_client *client = bedrock_client_create();

	socklen_t addrlen = sizeof(client->fd.addr);
	client->fd.fd = accept(fd->fd, &client->fd.addr.in, &addrlen);
	if (client->fd.fd < 0)
	{
		bedrock_log(LEVEL_CRIT, "Error accepting client - %s", strerror(errno));
		bedrock_client_free(client);
		return;
	}

	bedrock_log(LEVEL_DEBUG, "Accepted client");

	client->fd.read_handler = bedrock_client_read;
	client->fd.write_handler = bedrock_client_write;
}

void init_listener()
{
	static bedrock_fd fd;
	socklen_t opt = 1;

	fd.fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd.fd  < 0)
	{
		bedrock_log(LEVEL_CRIT, "Unable to create socket - %s", strerror(errno));
		abort();
	}

	fcntl(fd.fd, F_SETFL, fcntl(fd.fd, F_GETFL, 0) | O_NONBLOCK);

	setsockopt(fd.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (inet_pton(AF_INET, "0.0.0.0", &fd.addr.in4.sin_addr) != 1)
	{
		bedrock_log(LEVEL_CRIT, "Unable to call inet_pton - %s", strerror(errno));
		abort();
	}

	fd.addr.in4.sin_family = AF_INET;
	fd.addr.in4.sin_port = htons(25565);
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
	bedrock_io_set(&fd, OP_READ, 0);
}
