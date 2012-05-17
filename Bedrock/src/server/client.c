#include "server/bedrock.h"
#include "server/client.h"
#include "util/memory.h"
#include "io/io.h"
#include "packet/packet.h"
#include "compression/compression.h"
#include "util/endian.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>

bedrock_list client_list;
static bedrock_list exiting_client_list;

struct bedrock_client *client_create()
{
	struct bedrock_client *client = bedrock_malloc(sizeof(struct bedrock_client));
	client->out_buffer = bedrock_buffer_create(NULL, 0, BEDROCK_CLIENT_BUFFER_LENGTH);
	client->authenticated = STATE_UNAUTHENTICATED;
	bedrock_list_add(&client_list, client);
	return client;
}

bool client_load(struct bedrock_client *client)
{
	char path[PATH_MAX];
	int fd;
	struct stat file_info;
	unsigned char *file_base;
	compression_buffer *cb;
	nbt_tag *tag;

	bedrock_assert_ret(client != NULL && client->world != NULL, false);

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

	tag = nbt_parse(cb->buffer->data, cb->buffer->length);
	compression_decompress_end(cb);
	if (tag == NULL)
	{
		bedrock_log(LEVEL_CRIT, "client: Unable to NBT parse world information file %s", path);
		return false;
	}

	client->data = tag;
	bedrock_log(LEVEL_DEBUG, "client: Successfully loaded player information file for %s", client->name);

	return true;
}

static void client_free(struct bedrock_client *client)
{
	bedrock_log(LEVEL_DEBUG, "client: Exiting client from %s", client_get_ip(client));

	io_set(&client->fd.fd, 0, ~0);
	bedrock_fd_close(&client->fd);

	bedrock_list_del(&client_list, client);

	bedrock_buffer_free(client->out_buffer);
	bedrock_free(client);
}

void client_exit(struct bedrock_client *client)
{
	if (bedrock_list_has_data(&exiting_client_list, client) == false)
		bedrock_list_add(&exiting_client_list, client);
}

void client_process_exits()
{
	bedrock_node *n;

	LIST_FOREACH(&exiting_client_list, n)
	{
		struct bedrock_client *client = n->data;

		client_free(client);
	}

	bedrock_list_clear(&exiting_client_list);
}

void client_event_read(bedrock_fd *fd, void *data)
{
	struct bedrock_client *client = data;

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
	struct bedrock_client *client = data;
	int i;

	if (client->out_buffer->length == 0)
	{
		io_set(&client->fd, 0, OP_WRITE);
		if (client->fd.ops == 0)
			client_exit(client);
		return;
	}

	i = send(fd->fd, client->out_buffer->data, client->out_buffer->length, 0);
	if (i <= 0)
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Lost connection from client %s (%s)", *client->name ? client->name : "(unknown)", client_get_ip(client));
		io_set(fd, 0, OP_READ | OP_WRITE);
		client_exit(client);
		return;
	}

	client->out_buffer->length -= i;
	if (client->out_buffer->length > 0)
	{
		memmove(client->out_buffer->data, client->out_buffer->data + i, client->out_buffer->length);

		if (client->out_buffer->capacity > BEDROCK_CLIENT_BUFFER_LENGTH && client->out_buffer->length <= BEDROCK_CLIENT_BUFFER_LENGTH)
		{
			bedrock_log(LEVEL_DEBUG, "io: Resizing buffer for %s (%s) down to %d", *client->name ? client->name : "(unknown)", client_get_ip(client), BEDROCK_CLIENT_BUFFER_LENGTH);
			bedrock_buffer_resize(client->out_buffer, BEDROCK_CLIENT_BUFFER_LENGTH);
		}
	}
	else
	{
		io_set(&client->fd, 0, OP_WRITE);
		if (client->fd.ops == 0)
			client_exit(client);
	}
}

const char *client_get_ip(struct bedrock_client *client)
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

void client_send_header(struct bedrock_client *client, uint8_t header)
{
	bedrock_log(LEVEL_PACKET_DEBUG, "packet: Queueing packet header 0x%x for %s (%s)", header, *client->name ? client->name : "(unknown)", client_get_ip(client));
	client_send_int(client, &header, sizeof(header));
}

void client_send(struct bedrock_client *client, const void *data, size_t size)
{
	bedrock_assert(client != NULL && data != NULL);

	if (client->authenticated != STATE_BURSTING && client->out_buffer->length + size > BEDROCK_CLIENT_BUFFER_LENGTH)
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Send queue exceeded for %s (%s) - dropping client", *client->name ? client->name : "(unknown)", client_get_ip(client));
		client_exit(client);
		return;
	}

	bedrock_buffer_append(client->out_buffer, data, size);

	io_set(&client->fd, OP_WRITE, 0);
}

void client_send_int(struct bedrock_client *client, const void *data, size_t size)
{
	size_t old_len;

	bedrock_assert(client != NULL && data != NULL);

	old_len = client->out_buffer->length;
	client_send(client, data, size);
	if (old_len + size == client->out_buffer->length)
		convert_endianness(client->out_buffer->data + old_len, size);
}

void client_send_string(struct bedrock_client *client, const char *string)
{
	uint16_t len, i;

	bedrock_assert(client != NULL && string != NULL);

	len = strlen(string);

	client_send_int(client, &len, sizeof(len));

	if (client->authenticated != STATE_BURSTING && client->out_buffer->length + (len * 2) > BEDROCK_CLIENT_BUFFER_LENGTH)
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Send queue exceeded for %s (%s) - dropping client", *client->name ? client->name : "(unknown)", client_get_ip(client));
		client_exit(client);
		return;
	}

	bedrock_ensure_capacity(client->out_buffer, len * 2);

	for (i = 0; i < len; ++i)
	{
		client->out_buffer->data[client->out_buffer->length++] = 0;
		client->out_buffer->data[client->out_buffer->length++] = *string++;
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

double *client_get_pos_x(struct bedrock_client *client)
{
	return nbt_read(client->data, TAG_DOUBLE, 2, "Pos", 0);
}

double *client_get_pos_y(struct bedrock_client *client)
{
	return nbt_read(client->data, TAG_DOUBLE, 2, "Pos", 1);
}

double *client_get_pos_z(struct bedrock_client *client)
{
	return nbt_read(client->data, TAG_DOUBLE, 2, "Pos", 2);
}

static void client_send_chunks(struct bedrock_client *client, nbt_tag *column)
{
	uint8_t b;
	int8_t last_y = -1;
	uint16_t s, bitmask;
	nbt_tag *tag;
	bedrock_node *node;
	compression_buffer *buffer;
	struct nbt_tag_byte_array *blocks;

	client_send_header(client, MAP_COLUMN_ALLOCATION);
	client_send_int(client, nbt_read(column, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(column, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = 1;
	client_send_int(client, &b, sizeof(b));

	client_send_header(client, MAP_COLUMN);
	client_send_int(client, nbt_read(column, TAG_INT, 2, "Level", "xPos"), sizeof(uint32_t)); // X
	client_send_int(client, nbt_read(column, TAG_INT, 2, "Level", "zPos"), sizeof(uint32_t)); // Z
	b = 1;
	client_send_int(client, &b, sizeof(b)); // Ground up continuous

	tag = nbt_get(column, 2, "Level", "Sections");
	bedrock_assert_ret(tag != NULL && tag->type == TAG_LIST, ERROR_UNKNOWN);
	bitmask = 0;
	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		nbt_copy(sec, &b, sizeof(b), 1, "Y");
		bedrock_assert_ret((last_y + 1) == b, ERROR_UNKNOWN);
		last_y = b;
		bitmask |= 1 << b;
	}
	client_send_int(client, &bitmask, sizeof(bitmask)); // primary bit map

	bitmask = 0;
	client_send_int(client, &bitmask, sizeof(bitmask)); // add bit map

	buffer = compression_compress_init();
	bedrock_assert_ret(buffer, ERROR_UNKNOWN);

	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "Blocks");
		bedrock_assert_ret(blocks->length == 4096, ERROR_UNKNOWN);

		compression_compress_deflate(buffer, blocks->data, blocks->length);
	}

	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "Data");
		bedrock_assert_ret(blocks->length == 2048, ERROR_UNKNOWN);

		compression_compress_deflate(buffer, blocks->data, blocks->length);
	}

	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "BlockLight");
		bedrock_assert_ret(blocks->length == 2048, ERROR_UNKNOWN);

		compression_compress_deflate(buffer, blocks->data, blocks->length);
	}

	LIST_FOREACH(&tag->payload.tag_compound, node)
	{
		nbt_tag *sec = node->data;

		blocks = nbt_read(sec, TAG_BYTE_ARRAY, 1, "SkyLight");
		bedrock_assert_ret(blocks->length == 2048, ERROR_UNKNOWN);

		compression_compress_deflate(buffer, blocks->data, blocks->length);
	}

	blocks = nbt_read(column, TAG_BYTE_ARRAY, 2, "Level", "Biomes");
	bedrock_assert_ret(blocks->length == 256, ERROR_UNKNOWN);

	compression_compress_deflate(buffer, blocks->data, blocks->length);

	uint32_t ii = buffer->buffer->length;
	client_send_int(client, &ii, sizeof(ii)); // length
	ii = 0;
	client_send_int(client, &ii, sizeof(ii)); // not used

	client_send(client, buffer->buffer->data, buffer->buffer->length);

	compression_compress_end(buffer);
}

void client_update_chunks(struct bedrock_client *client)
{
	/* Update the chunks around the player. Used for when the player moves to a new chunk.
	 * client->columns contains a list of nbt_tag columns.
	 */
	int i;

	/* Player coords */
	double x = *client_get_pos_x(client), z = *client_get_pos_z(client);
	/* Column the player is in */
	printf("Finding what contains %d %d\n", (int)x, (int)z);
	nbt_tag *base_column = find_column_which_contains(client->world, x, z);
	bedrock_assert(base_column);

	// Send current column?
	client_send_chunks(client, base_column);

	for (i = 1; i < BEDROCK_VIEW_LENGTH; ++i)
	{
		int j;

		/* First, go from -i,i to i,i (exclusive) */
		for (j = -i; j < i; ++j)
		{
			nbt_tag *c = find_column_which_contains(client->world, x + (j * BEDROCK_BLOCKS_PER_CHUNK), z + (i * BEDROCK_BLOCKS_PER_CHUNK));
			//client_send_chunks(client, c);
		}

		/* Next, go from i,i to i,-i (exclusive) */
		for (j = i; j > -i; --j)
		{
			nbt_tag *c = find_column_which_contains(client->world, x + (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));
		}

		/* Next, go from i,-i to -i,-i (exclusive) */
		for (j = i; j > -i; --j)
		{
			nbt_tag *c = find_column_which_contains(client->world, x + (j * BEDROCK_BLOCKS_PER_CHUNK), z - (i * BEDROCK_BLOCKS_PER_CHUNK));
		}

		/* Next, go from -i,-i to -i,i (exclusive) */
		for (j = -i; j < i; ++j)
		{
			nbt_tag *c = find_column_which_contains(client->world, x - (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));
		}
	}
}
