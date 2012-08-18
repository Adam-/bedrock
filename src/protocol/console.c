#include "server/bedrock.h"
#include "io/io.h"
#include "util/memory.h"
#include "util/string.h"
#include "protocol/console.h"
#include "server/command.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#define SOCKET_NAME "./bedrock.console.socket"

bedrock_list console_list = LIST_INIT;

static struct bedrock_fd fd;
static bedrock_list exiting_client_list;

static void console_free(struct bedrock_console_client *client);

static int mem_find(const unsigned char *mem, size_t len, unsigned char val)
{
	size_t i;
	for (i = 0; i < len; ++i)
		if (mem[i] == val)
			break;
	return i;
}

static void console_client_read(struct bedrock_fd *fd, void *data)
{
	struct bedrock_console_client *client = data;
	int i;
	struct bedrock_command_source source;

	if (client->in_buffer_len == sizeof(client->in_buffer))
	{
		bedrock_log(LEVEL_INFO, "Receive queue exceeded for console client - dropping client");
		console_exit(client);
		return;
	}

	i = recv(fd->fd, client->in_buffer + client->in_buffer_len, sizeof(client->in_buffer) - client->in_buffer_len, 0);
	if (i <= 0)
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Lost connection from console client");
		io_set(fd, 0, OP_READ | OP_WRITE);
		console_exit(client);
		return;
	}
	client->in_buffer_len += i;

	source.user = NULL;
	source.console = client;

	while (io_has(fd, OP_READ) && (i = mem_find(client->in_buffer, client->in_buffer_len, '\n')))
	{
		client->in_buffer[i] = 0;

		command_run(&source, (char *) client->in_buffer);

		memmove(client->in_buffer, client->in_buffer + i + 1, client->in_buffer_len - i - 1);
		client->in_buffer_len -= i + 1;
	}
}

static void console_client_write(struct bedrock_fd *fd, void *data)
{
	struct bedrock_console_client *client = data;
	bedrock_node *node;
	const char *out_str;
	int out_str_len, i;

	if (client->out_buffer.count == 0)
	{
		io_set(&client->fd, 0, OP_WRITE);
		if (client->fd.ops == 0)
			console_exit(client);
		return;
	}

	node = client->out_buffer.head;
	out_str = node->data;
	out_str_len = strlen(out_str);

	i = send(fd->fd, out_str, out_str_len, 0);
	if (i <= 0)
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Lost connection from console client");
		io_set(fd, 0, OP_READ | OP_WRITE);
		console_exit(client);
		return;
	}

	bedrock_list_del_node(&client->out_buffer, node);
	bedrock_free(node);

	if (client->out_buffer.count == 0)
	{
		io_set(&client->fd, 0, OP_WRITE);

		if (client->fd.ops == 0)
			console_exit(client);
	}
}

static void accept_client(struct bedrock_fd *fd, void __attribute__((__unused__)) *unused)
{
	int client_fd;
	union
	{
		struct sockaddr addr;
		struct sockaddr_un un;
	} addr;
	socklen_t addrlen = sizeof(addr);
	socklen_t opt = 1;
	struct bedrock_console_client *client;

	client_fd = accept(fd->fd, &addr.addr, &addrlen);
	if (client_fd < 0)
	{
		bedrock_log(LEVEL_CRIT, "Error accepting client - %s", strerror(errno));
		return;
	}

	fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK);

	setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

	bedrock_log(LEVEL_INFO, "Accepted connection from console client");

	client = console_client_create();
	bedrock_fd_open(&client->fd, client_fd, FD_SOCKET, "client console fd");
	memcpy(&client->fd.addr, &addr, addrlen);

	client->fd.read_handler = console_client_read;
	client->fd.write_handler = console_client_write;

	client->fd.read_data = client;
	client->fd.write_data = client;

	io_set(&client->fd, OP_READ, 0);
}

void console_init()
{
	int listen_fd;
	socklen_t opt = 1;

	unlink(SOCKET_NAME);

	listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (listen_fd < 0)
	{
		bedrock_log(LEVEL_CRIT, "Unable to create console socket - %s", strerror(errno));
		abort();
	}

	bedrock_fd_open(&fd, listen_fd, FD_SOCKET, "console listen fd");

	fcntl(fd.fd, F_SETFL, fcntl(fd.fd, F_GETFL, 0) | O_NONBLOCK);

	setsockopt(fd.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	fd.addr.un.sun_family = AF_UNIX;
	strncpy(fd.addr.un.sun_path, SOCKET_NAME, sizeof(fd.addr.un.sun_path));

	if (bind(fd.fd, &fd.addr.in, sizeof(fd.addr.un)) < 0)
	{
		bedrock_log(LEVEL_CRIT, "Unable to bind console listener - %s", strerror(errno));
		abort();
	}

	if (listen(fd.fd, SOMAXCONN) < 0)
	{
		bedrock_log(LEVEL_CRIT, "Unable to listen - %s", strerror(errno));
		abort();
	}

	fd.read_handler = accept_client;

	io_set(&fd, OP_READ, 0);
}

void console_shutdown()
{
	bedrock_list_clear(&exiting_client_list);

	console_list.free = (bedrock_free_func) console_free;
	bedrock_list_clear(&console_list);

	bedrock_fd_close(&fd);
	unlink(SOCKET_NAME);
}

struct bedrock_console_client *console_client_create()
{
	struct bedrock_console_client *client = bedrock_malloc(sizeof(struct bedrock_console_client));
	client->out_buffer.free = bedrock_free;
	bedrock_list_add(&console_list, client);
	return client;
}

void console_exit(struct bedrock_console_client *client)
{
	if (bedrock_list_has_data(&exiting_client_list, client) == false)
		bedrock_list_add(&exiting_client_list, client);
}

static void console_free(struct bedrock_console_client *client)
{
	bedrock_fd_close(&client->fd);
	bedrock_list_del(&console_list, client);
	bedrock_list_clear(&client->out_buffer);
	bedrock_free(client);
}

void console_process_exits()
{
	bedrock_node *node;

	LIST_FOREACH(&exiting_client_list, node)
		console_free(node->data);
	bedrock_list_clear(&exiting_client_list);
}

void console_write(struct bedrock_console_client *client, const char *string)
{
	bedrock_list_add(&client->out_buffer, bedrock_strdup(string));
	io_set(&client->fd, OP_WRITE, 0);
}

