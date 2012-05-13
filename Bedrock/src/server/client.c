#include "server/bedrock.h"
#include "server/client.h"
#include "util/memory.h"
#include "io/io.h"
#include "packet/packet.h"

#include <arpa/inet.h>

bedrock_list client_list;
static bedrock_list exiting_client_list;

bedrock_client *bedrock_client_create()
{
	bedrock_client *client = bedrock_malloc(sizeof(bedrock_client));
	bedrock_list_add(&client_list, client);
	return client;
}

static void bedrock_client_free(bedrock_client *client)
{
	bedrock_log(LEVEL_DEBUG, "client: Exiting client from %s", bedrock_client_get_ip(client));

	bedrock_io_set(&client->fd.fd, 0, ~0);
	bedrock_fd_close(&client->fd);

	bedrock_list_del(&client_list, client);

	bedrock_free(client);
}

void bedrock_client_exit(bedrock_client *client)
{
	if (bedrock_list_has_data(&exiting_client_list, client) == false)
		bedrock_list_add(&exiting_client_list, client);
}

void bedrock_client_process_exits()
{
	bedrock_node *n;

	LIST_FOREACH(&exiting_client_list, n)
	{
		bedrock_client *client = n->data;

		bedrock_client_free(client);
	}

	bedrock_list_clear(&exiting_client_list);
}

void bedrock_client_event_read(bedrock_fd *fd, void *data)
{
	bedrock_client *client = data;

	int i = recv(fd->fd, client->in_buffer, sizeof(client->in_buffer) - client->in_buffer_len, 0);
	if (i <= 0)
	{
		bedrock_log(LEVEL_INFO, "Lost connection from client %s (%s)", *client->name ? client->name : "(unknown)", bedrock_client_get_ip(client));
		bedrock_io_set(fd, 0, OP_READ | OP_WRITE);
		bedrock_client_exit(client);
		return;
	}

	client->in_buffer_len += i;

	i = packet_parse(client, client->in_buffer, client->in_buffer_len);

	bedrock_assert(i != 0);
	bedrock_assert(i == -1 || i <= client->in_buffer_len);

	if (i > 0)
	{
		client->in_buffer_len -= i;

		if (client->in_buffer_len > 0)
			memmove(client->in_buffer, client->in_buffer + i, client->in_buffer_len);
	}
}

void bedrock_client_event_write(bedrock_fd *fd, void *data)
{
	bedrock_client *client = data;

	//if (client->out_buffer_len == 0)

	// If no more data is queued and read isn't wanted shut down
}

const char *bedrock_client_get_ip(bedrock_client *client)
{
	bedrock_assert_ret(client != NULL, NULL);

	if (*client->ip)
		return client->ip;

	switch (client->fd.addr.in.sa_family)
	{
		case AF_INET:
			return inet_ntop(AF_INET, &client->fd.addr.in4.sin_addr, client->ip, sizeof(client->ip));
		case AF_INET6:
			return inet_ntop(AF_INET6, &client->fd.addr.in6.sin6_addr, client->ip, sizeof(client->ip));
		default:
			break;
	}

	return "(unknown)";
}

void bedrock_client_send(bedrock_client *client, void *data, size_t size)
{
	if (client->out_buffer_len + size > sizeof(client->out_buffer))
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Send queue exceeded for %s (%s) - dropping client", *client->name ? client->name : "(unknown)", bedrock_client_get_ip(client));
		bedrock_client_exit(client);
		return;
	}

	memcpy(client->out_buffer, data, size);
	client->out_buffer_len += size;

	bedrock_assert(client->out_buffer_len <= sizeof(client->out_buffer));

	bedrock_io_set(&client->fd, OP_WRITE, 0);
}
