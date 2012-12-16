#include "server/bedrock.h"
#include "util/memory.h"
#include "util/string.h"
#include "util/io.h"
#include "protocol/console.h"
#include "server/command.h"

#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>

bedrock_list console_list = LIST_INIT;

static struct bedrock_fd fd;
static bedrock_list exiting_client_list;

static void console_exit(struct bedrock_console_client *client);
static void console_free(struct bedrock_console_client *client);

static struct bedrock_console_client *console_client_create()
{
	struct bedrock_console_client *client = bedrock_malloc(sizeof(struct bedrock_console_client));
	client->out_buffer.free = bedrock_free;
	bedrock_list_add(&console_list, client);
	return client;
}

static int mem_find(const unsigned char *mem, size_t len, unsigned char val)
{
	int i;
	for (i = 0; (unsigned) i < len; ++i)
		if (mem[i] == val)
			return i;
	return -1;
}

static void console_client_read(evutil_socket_t fd, short bedrock_attribute_unused events, void *data)
{
	struct bedrock_console_client *client = data;
	int i;
	struct bedrock_command_source source;

	if (client->in_buffer_len == sizeof(client->in_buffer))
	{
		bedrock_log(LEVEL_INFO, "Receive queue exceeded for console client - dropping client");

		io_disable(&client->fd.event_read);
		io_disable(&client->fd.event_write);

		return;
	}

	i = recv(fd, client->in_buffer + client->in_buffer_len, sizeof(client->in_buffer) - client->in_buffer_len, 0);
	if (i <= 0)
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Lost connection from console client");

		io_disable(&client->fd.event_read);
		io_disable(&client->fd.event_write);

		console_exit(client);
		return;
	}
	client->in_buffer_len += i;

	source.user = NULL;
	source.console = client;

	while (io_is_pending(&client->fd.event_read, EV_READ) && (i = mem_find(client->in_buffer, client->in_buffer_len, '\n')) > 0)
	{
		client->in_buffer[i] = 0;

		if (client->in_buffer[i - 1] == '\r')
			client->in_buffer[i - 1] = 0;

		command_run(&source, (char *) client->in_buffer);

		memmove(client->in_buffer, client->in_buffer + i + 1, client->in_buffer_len - i - 1);
		client->in_buffer_len -= i + 1;
	}
}

static void console_client_write(evutil_socket_t fd, short bedrock_attribute_unused events, void *data)
{
	struct bedrock_console_client *client = data;
	bedrock_node *node;
	const char *out_str;
	int out_str_len, i;

	if (client->out_buffer.count == 0)
	{
		io_disable(&client->fd.event_write);

		if (io_is_pending(&client->fd.event_read, EV_READ) == false)
			console_exit(client);
		return;
	}

	node = client->out_buffer.head;
	out_str = node->data;
	out_str_len = strlen(out_str);

	i = send(fd, out_str, out_str_len, 0);
	if (i <= 0)
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Lost connection from console client");

		io_disable(&client->fd.event_read);
		io_disable(&client->fd.event_write);

		console_exit(client);
		return;
	}

	bedrock_list_del_node(&client->out_buffer, node);
	bedrock_free(node);

	if (client->out_buffer.count == 0)
	{
		io_disable(&client->fd.event_write);

		if (io_is_pending(&client->fd.event_read, EV_READ) == false)
			console_exit(client);
	}
}

static void accept_client(evutil_socket_t fd, short bedrock_attribute_unused events, void bedrock_attribute_unused *data)
{
	int client_fd;
	union
	{
		struct sockaddr addr;
		struct sockaddr_in in;
	} addr;
	socklen_t addrlen = sizeof(addr);
	socklen_t opt = 1;
	struct bedrock_console_client *client;

	client_fd = accept(fd, &addr.addr, &addrlen);
	if (client_fd < 0)
	{
		bedrock_log(LEVEL_CRIT, "Error accepting client - %s", strerror(errno));
		return;
	}

	evutil_make_socket_nonblocking(client_fd);

	setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

	bedrock_log(LEVEL_INFO, "Accepted connection from console client");

	client = console_client_create();
	bedrock_fd_open(&client->fd, client_fd, FD_SOCKET, "client console fd");
	memcpy(&client->fd.addr, &addr, addrlen);

	io_assign(&client->fd.event_read, client->fd.fd, EV_PERSIST | EV_READ, console_client_read, client);
	io_assign(&client->fd.event_write, client->fd.fd, EV_PERSIST | EV_WRITE, console_client_write, client);

	io_enable(&client->fd.event_read);
}

void console_init()
{
	int listen_fd;
	socklen_t opt = 1;

	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
	{
		bedrock_log(LEVEL_CRIT, "Unable to create console socket - %s", strerror(errno));
		abort();
	}

	bedrock_fd_open(&fd, listen_fd, FD_SOCKET, "console listen fd");

	evutil_make_socket_nonblocking(fd.fd);

	setsockopt(fd.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	fd.addr.in4.sin_family = AF_INET;
	fd.addr.in4.sin_port = htons(23934); /* Random port, should write to a file and have the console read it? */
	inet_pton(AF_INET, "127.0.0.1", &fd.addr.in4.sin_addr);
	opt = sizeof(struct sockaddr_in);

	if (bind(fd.fd, &fd.addr.in, opt) < 0)
	{
		bedrock_log(LEVEL_CRIT, "Unable to bind console listener - %s", strerror(errno));
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

void console_shutdown()
{
	bedrock_list_clear(&exiting_client_list);

	console_list.free = (bedrock_free_func) console_free;
	bedrock_list_clear(&console_list);

	bedrock_fd_close(&fd);
}

static void console_exit(struct bedrock_console_client *client)
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
	io_enable(&client->fd.event_write);
}

