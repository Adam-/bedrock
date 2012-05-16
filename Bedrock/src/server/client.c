#include "server/bedrock.h"
#include "server/client.h"
#include "util/memory.h"
#include "io/io.h"
#include "packet/packet.h"
#include "compression/compression.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>

bedrock_list client_list;
static bedrock_list exiting_client_list;

bedrock_client *client_create()
{
	bedrock_client *client = bedrock_malloc(sizeof(bedrock_client));
	client->authenticated = STATE_UNAUTHENTICATED;
	bedrock_list_add(&client_list, client);
	return client;
}

bool client_load(bedrock_client *client)
{
	char path[PATH_MAX];
	int fd;
	struct stat file_info;
	unsigned char *file_base;
	bedrock_buffer *cb;
	nbt_tag *tag;

	bedrock_assert(client != NULL && client->world != NULL);

	snprintf(path, sizeof(path), "%s/players/%s.dat", client->world->path, client->name);

	fd = open(path, O_RDONLY);
	if (fd == -1)
	{
		bedrock_log(LEVEL_CRIT, "client: Unable to load player information for %s from %s - %s", client->name, path, strerror(errno));
		return false;
	}

	if (fstat(fd, &file_info) != 0)
	{
		bedrock_log(LEVEL_CRIT, "client: Unable to stat player information file %s - %s", path, strerror(errno));
		close(fd);
		return false;
	}

	file_base = mmap(NULL, file_info.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (file_base == MAP_FAILED)
	{
		bedrock_log(LEVEL_CRIT, "client: Unable to map player information file %s - %s", path, strerror(errno));
		close(fd);
		return false;
	}

	close(fd);

	cb = compression_decompress(file_base, file_info.st_size);
	munmap(file_base, file_info.st_size);
	if (cb == NULL)
	{
		bedrock_log(LEVEL_CRIT, "client: Unable to inflate player information file %s", path);
		return false;
	}

	tag = nbt_parse(cb->data, cb->length);
	compression_free_buffer(cb);
	if (tag == NULL)
	{
		bedrock_log(LEVEL_CRIT, "client: Unable to NBT parse world information file %s", path);
		return false;
	}

	client->data = tag;
	bedrock_log(LEVEL_DEBUG, "client: Successfully loaded player information file for %s", client->name);

	return true;
}

static void client_free(bedrock_client *client)
{
	bedrock_log(LEVEL_DEBUG, "client: Exiting client from %s", client_get_ip(client));

	io_set(&client->fd.fd, 0, ~0);
	bedrock_fd_close(&client->fd);

	bedrock_list_del(&client_list, client);

	bedrock_free(client);
}

void client_exit(bedrock_client *client)
{
	if (bedrock_list_has_data(&exiting_client_list, client) == false)
		bedrock_list_add(&exiting_client_list, client);
}

void client_process_exits()
{
	bedrock_node *n;

	LIST_FOREACH(&exiting_client_list, n)
	{
		bedrock_client *client = n->data;

		client_free(client);
	}

	bedrock_list_clear(&exiting_client_list);
}

void client_event_read(bedrock_fd *fd, void *data)
{
	bedrock_client *client = data;

	if (client->in_buffer_len == sizeof(client->in_buffer))
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Receive queue exceeded for %s (%s) - dropping client", *client->name ? client->name : "(unknown)", client_get_ip(client));
		client_exit(client);
		return;
	}

	int i = recv(fd->fd, client->in_buffer, sizeof(client->in_buffer) - client->in_buffer_len, 0);
	if (i <= 0)
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Lost connection from client %s (%s)", *client->name ? client->name : "(unknown)", client_get_ip(client));
		io_set(fd, 0, OP_READ | OP_WRITE);
		client_exit(client);
		return;
	}

	client->in_buffer_len += i;

	while ((i = packet_parse(client, client->in_buffer, client->in_buffer_len)) > 0)
	{
		bedrock_assert(i <= client->in_buffer_len);

		client->in_buffer_len -= i;

		if (client->in_buffer_len > 0)
			memmove(client->in_buffer, client->in_buffer + i, client->in_buffer_len);
	}
}

void client_event_write(bedrock_fd *fd, void *data)
{
	bedrock_client *client = data;
	int i;

	if (client->out_buffer_len == 0)
	{
		io_set(&client->fd, 0, OP_WRITE);
		if (client->fd.ops == 0)
			client_exit(client);
		return;
	}

	i = send(fd->fd, client->out_buffer, client->out_buffer_len, 0);
	if (i <= 0)
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Lost connection from client %s (%s)", *client->name ? client->name : "(unknown)", client_get_ip(client));
		io_set(fd, 0, OP_READ | OP_WRITE);
		client_exit(client);
		return;
	}

	client->out_buffer_len -= i;
	if (client->out_buffer_len > 0)
	{
		memmove(client->out_buffer, client->out_buffer + i, client->out_buffer_len);
	}
	else
	{
		io_set(&client->fd, 0, OP_WRITE);
		if (client->fd.ops == 0)
			client_exit(client);
	}
}

const char *client_get_ip(bedrock_client *client)
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

void client_send_header(bedrock_client *client, uint8_t header)
{
	bedrock_log(LEVEL_PACKET_DEBUG, "packet: Queueing packet header 0x%x for %s (%s)", header, *client->name ? client->name : "(unknown)", client_get_ip(client));
	client_send_int(client, &header, sizeof(header));
}

void client_send(bedrock_client *client, const void *data, size_t size)
{
	bedrock_assert(client != NULL && data != NULL);

	if (client->out_buffer_len + size > sizeof(client->out_buffer))
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Send queue exceeded for %s (%s) - dropping client", *client->name ? client->name : "(unknown)", client_get_ip(client));
		client_exit(client);
		return;
	}

	memcpy(client->out_buffer + client->out_buffer_len, data, size);
	client->out_buffer_len += size;

	bedrock_assert(client->out_buffer_len <= sizeof(client->out_buffer));

	io_set(&client->fd, OP_WRITE, 0);
}

void client_send_int(bedrock_client *client, const void *data, size_t size)
{
	size_t old_len;

	bedrock_assert(client != NULL && data != NULL);

	old_len = client->out_buffer_len;
	client_send(client, data, size);
	if (old_len + size == client->out_buffer_len)
		convert_endianness(client->out_buffer + old_len, size);
}

void client_send_string(bedrock_client *client, const char *string)
{
	uint16_t len, i;

	bedrock_assert(client != NULL && string != NULL);

	len = strlen(string);

	client_send_int(client, &len, sizeof(len));

	if (client->out_buffer_len + (len * 2) > sizeof(client->out_buffer))
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Send queue exceeded for %s (%s) - dropping client", *client->name ? client->name : "(unknown)", client_get_ip(client));
		client_exit(client);
		return;
	}

	for (i = 0; i < len; ++i)
	{
		client->out_buffer[client->out_buffer_len++] = 0;
		client->out_buffer[client->out_buffer_len++] = *string++;
	}
}

bool client_valid_username(const char *name)
{
	int i, len;

	bedrock_assert_ret(name != NULL, false);

	len = strlen(name);

	if (len < BEDROCK_USERNAME_MIN || len > BEDROCK_USERNAME_MAX - 1)
		return false;

	for (i = 0; i < len; ++i)
		if (!isalnum(name[i]) && name[i] != '_')
			return false;

	return true;
}
