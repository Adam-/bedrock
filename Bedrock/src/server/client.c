#include "server/bedrock.h"
#include "server/client.h"
#include "util/memory.h"
#include "io/io.h"
#include "packet/packet.h"
#include "compression/compression.h"
#include "util/endian.h"
#include "nbt/nbt.h"
#include "packet/packet_spawn_point.h"
#include "packet/packet_position_and_look.h"
#include "packet/packet_player_list_item.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>

bedrock_list client_list;
uint32_t entity_id = 0;
static bedrock_list exiting_client_list;

struct bedrock_client *client_create()
{
	struct bedrock_client *client = bedrock_malloc(sizeof(struct bedrock_client));
	client->id = ++entity_id;
	client->authenticated = STATE_UNAUTHENTICATED;
	client->out_buffer = bedrock_buffer_create(NULL, 0, BEDROCK_CLIENT_SENDQ_LENGTH);
	bedrock_list_add(&client_list, client);
	return client;
}

struct bedrock_client *client_find(const char *name)
{
	bedrock_node *node;

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *client = node->data;

		if (!strcmp(client->name, name))
			return client;
	}

	return NULL;
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
	bedrock_node *node;

	if (client->authenticated == STATE_AUTHENTICATED)
	{
		LIST_FOREACH(&client_list, node)
		{
			struct bedrock_client *c = node->data;

			if (c->authenticated == STATE_AUTHENTICATED && c != client)
			{
				packet_send_player_list_item(c, client->name, false, 0);
				packet_send_chat_message(c, "%s left the game", client->name);
			}
		}
	}

	LIST_FOREACH(&client->players, node)
	{
		struct bedrock_client *c = node->data;

		bedrock_assert(bedrock_list_del(&c->players, client));
		packet_sent_destroy_entity_player(c, client);
	}
	bedrock_list_clear(&client->players);

	bedrock_list_clear(&client->columns);

	bedrock_log(LEVEL_DEBUG, "client: Exiting client %s from %s", *client->name ? client->name : "(unknown)", client_get_ip(client));

	io_set(&client->fd.fd, 0, ~0);
	bedrock_fd_close(&client->fd);

	bedrock_list_del(&client_list, client);

	bedrock_buffer_free(client->out_buffer);
	bedrock_free(client);
}

void client_exit(struct bedrock_client *client)
{
	if (bedrock_list_has_data(&exiting_client_list, client) == false)
	{
		bedrock_list_add(&exiting_client_list, client);
		io_set(&client->fd.fd, 0, OP_READ);
	}
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
	}
	else
	{
		io_set(&client->fd, 0, OP_WRITE);
		if (client->fd.ops == 0)
			client_exit(client);
	}

	if (client->out_buffer->capacity > BEDROCK_CLIENT_SENDQ_LENGTH && client->out_buffer->length <= BEDROCK_CLIENT_SENDQ_LENGTH)
	{
		bedrock_log(LEVEL_DEBUG, "io: Resizing buffer for %s (%s) down to %d", *client->name ? client->name : "(unknown)", client_get_ip(client), BEDROCK_CLIENT_SENDQ_LENGTH);
		bedrock_buffer_resize(client->out_buffer, BEDROCK_CLIENT_SENDQ_LENGTH);
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

	if (client->authenticated != STATE_BURSTING && client->out_buffer->length + size > BEDROCK_CLIENT_SENDQ_LENGTH)
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

	if (client->authenticated != STATE_BURSTING && client->out_buffer->length + (len * 2) > BEDROCK_CLIENT_SENDQ_LENGTH)
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

float *client_get_yaw(struct bedrock_client *client)
{
	return nbt_read(client->data, TAG_FLOAT, 2, "Rotation", 0);
}

float *client_get_pitch(struct bedrock_client *client)
{
	return nbt_read(client->data, TAG_FLOAT, 2, "Rotation", 1);
}

uint8_t *client_get_on_ground(struct bedrock_client *client)
{
	return nbt_read(client->data, TAG_BYTE, 1, "OnGround");
}

void client_update_chunks(struct bedrock_client *client)
{
	/* Update the chunks around the player. Used for when the player moves to a new chunk.
	 * client->columns contains a list of nbt_tag columns.
	 */

	int i;
	struct bedrock_region *region;
	nbt_tag *c;
	bedrock_node *node, *node2;
	/* Player coords */
	double x = *client_get_pos_x(client), z = *client_get_pos_z(client);
	double player_x = x / BEDROCK_BLOCKS_PER_CHUNK, player_z = z / BEDROCK_BLOCKS_PER_CHUNK;

	player_x = (int) (player_x >= 0 ? ceil(player_x) : floor(player_x));
	player_z = (int) (player_z >= 0 ? ceil(player_z) : floor(player_z));

	/* Find columns we can delete */
	LIST_FOREACH_SAFE(&client->columns, node, node2)
	{
		nbt_tag *c = node->data;

		int32_t *column_x = nbt_read(c, TAG_INT, 2, "Level", "xPos"),
				*column_z = nbt_read(c, TAG_INT, 2, "Level", "zPos");

		if (abs(*column_x - player_x) > BEDROCK_VIEW_LENGTH || abs(*column_z - player_z) > BEDROCK_VIEW_LENGTH)
		{
			bedrock_list_del_node(&client->columns, node);

			bedrock_log(LEVEL_DEBUG, "client: Unallocating column %d, %d for %s", *column_x, *column_z, client->name);

			packet_send_column_allocation(client, c, false);
			packet_send_column_empty(client, c);
		}
	}

	/* Column the player is in */
	region = find_region_which_contains(client->world, x, z);
	bedrock_assert(region);

	c = find_column_which_contains(region, x, z);
	bedrock_assert(c);

	if (bedrock_list_has_data(&client->columns, c) == false)
	{
		packet_send_column_allocation(client, c, true);
		packet_send_column(client, c);
		bedrock_list_add(&client->columns, c);
	}

	for (i = 1; i < BEDROCK_VIEW_LENGTH; ++i)
	{
		int j;

		/* First, go from -i,i to i,i (exclusive) */
		for (j = -i; j < i; ++j)
		{
			region = find_region_which_contains(client->world, x + (j * BEDROCK_BLOCKS_PER_CHUNK), z + (i * BEDROCK_BLOCKS_PER_CHUNK));
			bedrock_assert(region);
			c = find_column_which_contains(region, x + (j * BEDROCK_BLOCKS_PER_CHUNK), z + (i * BEDROCK_BLOCKS_PER_CHUNK));

			if (bedrock_list_has_data(&client->columns, c))
				continue;

			packet_send_column_allocation(client, c, true);
			packet_send_column(client, c);
			bedrock_list_add(&client->columns, c);
		}

		/* Next, go from i,i to i,-i (exclusive) */
		for (j = i; j > -i; --j)
		{
			region = find_region_which_contains(client->world, x + (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));
			c = find_column_which_contains(region, x + (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));

			if (bedrock_list_has_data(&client->columns, c))
				continue;

			packet_send_column_allocation(client, c, true);
			packet_send_column(client, c);
			bedrock_list_add(&client->columns, c);
		}

		/* Next, go from i,-i to -i,-i (exclusive) */
		for (j = i; j > -i; --j)
		{
			region = find_region_which_contains(client->world, x + (j * BEDROCK_BLOCKS_PER_CHUNK), z - (i * BEDROCK_BLOCKS_PER_CHUNK));
			c = find_column_which_contains(region, x + (j * BEDROCK_BLOCKS_PER_CHUNK), z - (i * BEDROCK_BLOCKS_PER_CHUNK));

			if (bedrock_list_has_data(&client->columns, c))
				continue;

			packet_send_column_allocation(client, c, true);
			packet_send_column(client, c);
			bedrock_list_add(&client->columns, c);
		}

		/* Next, go from -i,-i to -i,i (exclusive) */
		for (j = -i; j < i; ++j)
		{
			region = find_region_which_contains(client->world, x - (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));
			c = find_column_which_contains(region, x - (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));

			if (bedrock_list_has_data(&client->columns, c))
				continue;

			packet_send_column_allocation(client, c, true);
			packet_send_column(client, c);
			bedrock_list_add(&client->columns, c);
		}
	}
}

void client_update_players(struct bedrock_client *client)
{
	/* Update the players this player knows about. Used for when a player
	 * moves to a new chunk.
	 */
	bedrock_node *node;

	/* Chunk coords player is in */
	double player_x = *client_get_pos_x(client) / BEDROCK_BLOCKS_PER_CHUNK, player_z = *client_get_pos_z(client) / BEDROCK_BLOCKS_PER_CHUNK;
	player_x = (int) (player_x >= 0 ? ceil(player_x) : floor(player_x));
	player_z = (int) (player_z >= 0 ? ceil(player_z) : floor(player_z));

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;
		double c_x, c_z;

		if (c->authenticated != STATE_AUTHENTICATED || c == client)
			continue;

		c_x = *client_get_pos_x(c) / BEDROCK_BLOCKS_PER_CHUNK, c_z = *client_get_pos_z(c) / BEDROCK_BLOCKS_PER_CHUNK;
		c_x = (int) (c_x >= 0 ? ceil(c_x) : floor(c_x));
		c_z = (int) (c_z >= 0 ? ceil(c_z) : floor(c_z));

		if (abs(player_x - c_x) <= BEDROCK_VIEW_LENGTH || abs(player_z - c_z) <= BEDROCK_VIEW_LENGTH)
		{
			/* Player c is in range of player client */
			if (bedrock_list_has_data(&client->players, c) == false)
			{
				/* Not already being tracked */
				bedrock_list_add(&client->players, c);
				packet_send_spawn_named_entity(client, c);

				bedrock_assert(bedrock_list_has_data(&c->players, client) == false);
				bedrock_list_add(&c->players, client);
				packet_send_spawn_named_entity(c, client);
			}
		}
		else
		{
			/* Player c is not in range of player client */
			if (bedrock_list_del(&client->players, c) != NULL)
			{
				/* And being tracked */
				packet_sent_destroy_entity_player(client, c);

				bedrock_assert(bedrock_list_del(&c->players, client) != NULL);
				packet_sent_destroy_entity_player(client, c);
			}
		}
	}
}

/* Called after a player moves, to inform other players about it */
void client_update_position(struct bedrock_client *client, double x, double y, double z, float yaw, float pitch, uint8_t on_ground)
{
	double old_x = *client_get_pos_x(client), old_y = *client_get_pos_y(client), old_z = *client_get_pos_z(client);
	float old_yaw = *client_get_yaw(client), old_pitch = *client_get_pitch(client);
	uint8_t old_on_ground = *client_get_on_ground(client);

	int8_t c_x, c_y, c_z, new_y, new_p;

	bedrock_node *node;

	if (old_x == x && old_y == y && old_z == z && old_pitch == pitch && old_yaw == yaw && old_on_ground == on_ground)
		return;

	if (old_x != x)
		nbt_set(client->data, TAG_DOUBLE, &x, sizeof(x), 2, "Pos", 0);
	if (old_y != y)
		nbt_set(client->data, TAG_DOUBLE, &y, sizeof(y), 2, "Pos", 1);
	if (old_z != z)
		nbt_set(client->data, TAG_DOUBLE, &z, sizeof(z), 2, "Pos", 2);
	if (old_yaw != yaw)
		nbt_set(client->data, TAG_FLOAT, &yaw, sizeof(yaw), 2, "Rotation", 0);
	if (old_pitch != pitch)
		nbt_set(client->data, TAG_FLOAT, &pitch, sizeof(pitch), 2, "Rotation", 1);

	c_x = ((int) x - (int) old_x) * 32;
	c_y = ((int) y - (int) old_y) * 32;
	c_z = ((int) z - (int) old_z) * 32;
	new_y = yaw;
	new_p = pitch;

	if (c_x == 0 && c_y == 0 && c_z == 0 && old_pitch == pitch && old_yaw == yaw)
		return;

	LIST_FOREACH(&client->players, node)
	{
		struct bedrock_client *c = node->data;

		client_send_header(c, 0x21);
		client_send_int(c, &client->id, sizeof(client->id));
		client_send_int(c, &c_x, sizeof(c_x));
		client_send_int(c, &c_y, sizeof(c_y));
		client_send_int(c, &c_z, sizeof(c_z));
		client_send_int(c, &new_y, sizeof(new_y));
		client_send_int(c, &new_p, sizeof(new_p));
	}
}

/* Right after a successful login, start the login sequence */
void client_send_login_sequence(struct bedrock_client *client)
{
	int32_t *spawn_x, *spawn_y, *spawn_z;
	bedrock_node *node;

	bedrock_assert(client->authenticated == STATE_BURSTING);

	/* Send world spawn point */
	spawn_x = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnX");
	spawn_y = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnY");
	spawn_z = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnZ");
	packet_send_spawn_point(client, *spawn_x, *spawn_y, *spawn_z);

	/* Send chunks */
	client_update_chunks(client);

	/* Send player position */
	packet_send_position_and_look(client, client);

	/* Send the client itself */
	packet_send_player_list_item(client, client->name, true, 0);
	/* Send the player lists */
	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;

		if (c->authenticated == STATE_AUTHENTICATED)
		{
			/* Send this new client to every client that is authenticated */
			packet_send_player_list_item(c, client->name, true, 0);
			/* Send this client to the new client */
			packet_send_player_list_item(client, c->name, true, 0);

			packet_send_chat_message(c, "%s joined the game", client->name);
		}
	}

	/* Send nearby players */
	client_update_players(client);

	client->authenticated = STATE_AUTHENTICATED;

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;

		if (c->authenticated != STATE_AUTHENTICATED || c == client)
			continue;

		client_update_players(c);
	}
}
