#include "server/bedrock.h"
#include "server/client.h"
#include "server/column.h"
#include "io/io.h"
#include "server/packet.h"
#include "compression/compression.h"
#include "util/endian.h"
#include "nbt/nbt.h"
#include "packet/packet_chat_message.h"
#include "packet/packet_collect_item.h"
#include "packet/packet_column.h"
#include "packet/packet_column_allocation.h"
#include "packet/packet_destroy_entity.h"
#include "packet/packet_entity_head_look.h"
#include "packet/packet_entity_teleport.h"
#include "packet/packet_position_and_look.h"
#include "packet/packet_player_list_item.h"
#include "packet/packet_spawn_named_entity.h"
#include "packet/packet_spawn_point.h"
#include "packet/packet_set_slot.h"
#include "packet/packet_time.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <math.h>

#define PLAYER_BUFFER_SIZE 4096

bedrock_list client_list = LIST_INIT;
int authenticated_client_count = 0;
struct bedrock_memory_pool client_pool = BEDROCK_MEMORY_POOL_INIT("client memory pool");

static bedrock_list exiting_client_list;

struct bedrock_client *client_create()
{
	struct bedrock_client *client = bedrock_malloc_pool(&client_pool, sizeof(struct bedrock_client));
	client->id = ++entity_id;
	client->authenticated = STATE_UNAUTHENTICATED;
	client->out_buffer = bedrock_buffer_create(&client_pool, NULL, 0, BEDROCK_CLIENT_SEND_SIZE);
	client->columns.pool = &client_pool;
	client->players.pool = &client_pool;
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

	bedrock_assert(client != NULL && client->world != NULL, return false);

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

	cb = compression_decompress(&client_pool, PLAYER_BUFFER_SIZE, file_base, file_info.st_size);
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
				packet_send_player_list_item(c, client, false);
				packet_send_chat_message(c, "%s left the game", client->name);
			}
		}
	}

	if (client->authenticated >= STATE_BURSTING)
	{
		--authenticated_client_count;
		bedrock_assert(authenticated_client_count >= 0, authenticated_client_count = 0);
	}

	LIST_FOREACH(&client->players, node)
	{
		struct bedrock_client *c = node->data;

		bedrock_assert(bedrock_list_del(&c->players, client), ;);
		packet_send_destroy_entity_player(c, client);
	}
	bedrock_list_clear(&client->players);

	LIST_FOREACH(&client->columns, node)
	{
		struct bedrock_column *c = node->data;

		bedrock_list_del(&c->players, client);

		--c->region->player_column_count;

		if (c->region->player_column_count == 0)
			region_queue_free(c->region);
	}
	bedrock_list_clear(&client->columns);

	bedrock_log(LEVEL_DEBUG, "client: Exiting client %s from %s", *client->name ? client->name : "(unknown)", client_get_ip(client));

	io_set(&client->fd, 0, ~0);
	bedrock_fd_close(&client->fd);

	bedrock_list_del(&client_list, client);

	if (client->data != NULL)
		nbt_free(client->data);
	bedrock_buffer_free(client->out_buffer);
	bedrock_free_pool(&client_pool, client);
}

void client_exit(struct bedrock_client *client)
{
	if (bedrock_list_has_data(&exiting_client_list, client) == false)
	{
		bedrock_list_add(&exiting_client_list, client);
		io_set(&client->fd, 0, OP_READ);
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

	while (io_has(fd, OP_READ) && (i = packet_parse(client, client->in_buffer, client->in_buffer_len)) > 0)
	{
		bedrock_assert((size_t) i <= client->in_buffer_len, break);

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
		{
			client_exit(client);
			return;
		}
	}

	bedrock_buffer_check_capacity(client->out_buffer, BEDROCK_CLIENT_SEND_SIZE);
}

const char *client_get_ip(struct bedrock_client *client)
{
	bedrock_assert(client != NULL, return NULL);

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
	bedrock_assert(client != NULL && data != NULL, return);

	bedrock_buffer_append(client->out_buffer, data, size);

	io_set(&client->fd, OP_WRITE, 0);
}

void client_send_int(struct bedrock_client *client, const void *data, size_t size)
{
	size_t old_len;

	bedrock_assert(client != NULL && data != NULL, return);

	old_len = client->out_buffer->length;
	client_send(client, data, size);
	if (old_len + size == client->out_buffer->length)
		convert_endianness(client->out_buffer->data + old_len, size);
}

void client_send_string(struct bedrock_client *client, const char *string)
{
	uint16_t len, i;

	bedrock_assert(client != NULL && string != NULL, return);

	len = strlen(string);

	client_send_int(client, &len, sizeof(len));

	bedrock_buffer_ensure_capacity(client->out_buffer, len * 2);

	for (i = 0; i < len; ++i)
	{
		client->out_buffer->data[client->out_buffer->length++] = 0;
		client->out_buffer->data[client->out_buffer->length++] = *string++;
	}
}

bool client_valid_username(const char *name)
{
	int i, len;

	bedrock_assert(name != NULL, return false);

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

nbt_tag *client_get_inventory_tag(struct bedrock_client *client, uint8_t slot)
{
	bedrock_node *node;

	LIST_FOREACH(&nbt_get(client->data, TAG_LIST, 1, "Inventory")->payload.tag_list, node)
	{
		nbt_tag *c = node->data;
		uint8_t *s = nbt_read(c, TAG_BYTE, 1, "Slot");

		if (*s == slot)
			return c;
	}

	return NULL;
}

bool client_can_add_inventory_item(struct bedrock_client *client, struct bedrock_item *item)
{
	bedrock_node *node;
	int i = -1;

	LIST_FOREACH(&nbt_get(client->data, TAG_LIST, 1, "Inventory")->payload.tag_list, node)
	{
		nbt_tag *c = node->data;
		uint16_t *id = nbt_read(c, TAG_SHORT, 1, "id");
		uint8_t *count = nbt_read(c, TAG_BYTE, 1, "Count");
		uint8_t *slot = nbt_read(c, TAG_BYTE, 1, "Slot");

		if (*id == item->id && *count < BEDROCK_MAX_ITEMS_PER_STACK)
			return true;
		else if (++i != *slot)
			return true;
	}

	if (i == 36) // Number of inventory slots
		return false;
	return true;
}

void client_add_inventory_item(struct bedrock_client *client, struct bedrock_item *item)
{
	bedrock_list *list = &nbt_get(client->data, TAG_LIST, 1, "Inventory")->payload.tag_list;
	bedrock_node *node;
	int i = -1;
	nbt_tag *c, *item_tag;
	uint8_t pos;

	LIST_FOREACH(list, node)
	{
		c = node->data;
		uint16_t *id = nbt_read(c, TAG_SHORT, 1, "id");
		uint8_t *count = nbt_read(c, TAG_BYTE, 1, "Count");
		uint8_t *slot = nbt_read(c, TAG_BYTE, 1, "Slot");

		++i;

		if (*id == item->id && *count < BEDROCK_MAX_ITEMS_PER_STACK)
		{
			++(*count);

			pos = *slot;
			if (pos >= 0 && pos <= 8)
				pos += 36;
			packet_send_set_slot(client, WINDOW_INVENTORY, pos, item, *count, 0);

			return;
		}
		else if (i != *slot)
			break;
	}

	if (i == 36) // Number of inventory slots
		return;

	pos = i;
	if (pos >= 0 && pos <= 8)
		pos += 36;
	packet_send_set_slot(client, WINDOW_INVENTORY, pos, item, 1, 0);

	item_tag = bedrock_malloc(sizeof(nbt_tag));
	item_tag->owner = nbt_get(client->data, TAG_LIST, 1, "Inventory");
	item_tag->type = TAG_COMPOUND;

	nbt_add(item_tag, TAG_SHORT, "id", &item->id, sizeof(item->id));
	uint16_t d = 0; // XXX
	nbt_add(item_tag, TAG_SHORT, "Damage", &d, sizeof(d));
	uint8_t b = 1;
	nbt_add(item_tag, TAG_BYTE, "Count", &b, sizeof(b));
	b = i;
	nbt_add(item_tag, TAG_BYTE, "Slot", &b, sizeof(b));

	// Insert slot i
	LIST_FOREACH(list, node)
	{
		c = node->data;
		uint8_t *slot = nbt_read(c, TAG_BYTE, 1, "Slot");

		if (*slot > i)
		{
			// Insert before c
			bedrock_list_add_node_before(list, bedrock_malloc_pool(list->pool, sizeof(bedrock_node)), node, item_tag);
			return;
		}
	}

	// Insert at the end
	bedrock_list_add(list, item_tag);
}

void client_update_chunks(struct bedrock_client *client)
{
	/* Update the chunks around the player. Used for when the player moves to a new chunk.
	 * client->columns contains a list of nbt_tag columns.
	 */

	int i;
	struct bedrock_region *region;
	struct bedrock_column *c;
	bedrock_node *node, *node2;
	/* Player coords */
	double x = *client_get_pos_x(client), z = *client_get_pos_z(client);
	double player_x = x / BEDROCK_BLOCKS_PER_CHUNK, player_z = z / BEDROCK_BLOCKS_PER_CHUNK;

	player_x = (int) (player_x >= 0 ? ceil(player_x) : floor(player_x));
	player_z = (int) (player_z >= 0 ? ceil(player_z) : floor(player_z));

	/* Find columns we can delete */
	LIST_FOREACH_SAFE(&client->columns, node, node2)
	{
		c = node->data;

		if (abs(c->x - player_x) > BEDROCK_VIEW_LENGTH || abs(c->z - player_z) > BEDROCK_VIEW_LENGTH)
		{
			bedrock_list_del_node(&client->columns, node);
			bedrock_free_pool(client->columns.pool, node);

			bedrock_list_del(&c->players, client);

			bedrock_log(LEVEL_COLUMN, "client: Unallocating column %d, %d for %s", c->x, c->z, client->name);

			--c->region->player_column_count;

			packet_send_column_allocation(client, c, false);
			packet_send_column_empty(client, c);

			if (c->region->player_column_count == 0)
				region_queue_free(c->region);
		}
	}

	/* Column the player is in */
	region = find_region_which_contains(client->world, x, z);
	bedrock_assert(region != NULL, return);

	client->column = c = find_column_which_contains(region, x, z);

	if (c != NULL)
		if (bedrock_list_has_data(&client->columns, c) == false)
		{
			bedrock_log(LEVEL_COLUMN, "client: Allocating column %d, %d for %s", c->x, c->z, client->name);

			++region->player_column_count;

			packet_send_column_allocation(client, c, true);
			packet_send_column(client, c);

			bedrock_list_add(&client->columns, c);
			bedrock_list_add(&c->players, client);

			/* Loading the column the player is in on a bursting player, finish burst */
			if (client->authenticated == STATE_BURSTING)
				client_finish_login_sequence(client);
		}

	for (i = 1; i < BEDROCK_VIEW_LENGTH; ++i)
	{
		int j;

		/* First, go from -i,i to i,i (exclusive) */
		for (j = -i; j < i; ++j)
		{
			region = find_region_which_contains(client->world, x + (j * BEDROCK_BLOCKS_PER_CHUNK), z + (i * BEDROCK_BLOCKS_PER_CHUNK));
			bedrock_assert(region != NULL, continue);
			c = find_column_which_contains(region, x + (j * BEDROCK_BLOCKS_PER_CHUNK), z + (i * BEDROCK_BLOCKS_PER_CHUNK));

			if (c == NULL || bedrock_list_has_data(&client->columns, c))
				continue;

			bedrock_log(LEVEL_COLUMN, "client: Allocating column %d, %d for %s", c->x, c->z, client->name);

			++region->player_column_count;

			packet_send_column_allocation(client, c, true);
			packet_send_column(client, c);

			bedrock_list_add(&client->columns, c);
			bedrock_list_add(&c->players, client);
		}

		/* Next, go from i,i to i,-i (exclusive) */
		for (j = i; j > -i; --j)
		{
			region = find_region_which_contains(client->world, x + (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));
			bedrock_assert(region != NULL, continue);
			c = find_column_which_contains(region, x + (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));

			if (c == NULL || bedrock_list_has_data(&client->columns, c))
				continue;

			bedrock_log(LEVEL_COLUMN, "client: Allocating column %d, %d for %s", c->x, c->z, client->name);

			++region->player_column_count;

			packet_send_column_allocation(client, c, true);
			packet_send_column(client, c);

			bedrock_list_add(&client->columns, c);
			bedrock_list_add(&c->players, client);
		}

		/* Next, go from i,-i to -i,-i (exclusive) */
		for (j = i; j > -i; --j)
		{
			region = find_region_which_contains(client->world, x + (j * BEDROCK_BLOCKS_PER_CHUNK), z - (i * BEDROCK_BLOCKS_PER_CHUNK));
			bedrock_assert(region != NULL, continue);
			c = find_column_which_contains(region, x + (j * BEDROCK_BLOCKS_PER_CHUNK), z - (i * BEDROCK_BLOCKS_PER_CHUNK));

			if (c == NULL || bedrock_list_has_data(&client->columns, c))
				continue;

			bedrock_log(LEVEL_COLUMN, "client: Allocating column %d, %d for %s", c->x, c->z, client->name);

			++region->player_column_count;

			packet_send_column_allocation(client, c, true);
			packet_send_column(client, c);

			bedrock_list_add(&client->columns, c);
			bedrock_list_add(&c->players, client);
		}

		/* Next, go from -i,-i to -i,i (exclusive) */
		for (j = -i; j < i; ++j)
		{
			region = find_region_which_contains(client->world, x - (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));
			bedrock_assert(region != NULL, continue);
			c = find_column_which_contains(region, x - (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));

			if (c == NULL || bedrock_list_has_data(&client->columns, c))
				continue;

			bedrock_log(LEVEL_COLUMN, "client: Allocating column %d, %d for %s", c->x, c->z, client->name);

			++region->player_column_count;

			packet_send_column_allocation(client, c, true);
			packet_send_column(client, c);

			bedrock_list_add(&client->columns, c);
			bedrock_list_add(&c->players, client);
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

				bedrock_assert(bedrock_list_has_data(&c->players, client) == false, ;);
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
				packet_send_destroy_entity_player(client, c);

				bedrock_assert(bedrock_list_del(&c->players, client) != NULL, ;);
				packet_send_destroy_entity_player(client, c);
			}
		}
	}
}

/* Called to update a players position */
void client_update_position(struct bedrock_client *client, double x, double y, double z, float yaw, float pitch, double stance, uint8_t on_ground)
{
	double old_x = *client_get_pos_x(client), old_y = *client_get_pos_y(client), old_z = *client_get_pos_z(client);
	float old_yaw = *client_get_yaw(client), old_pitch = *client_get_pitch(client);
	double old_stance = client->stance;
	uint8_t old_on_ground = *client_get_on_ground(client);

	double old_column_x = old_x / BEDROCK_BLOCKS_PER_CHUNK, old_column_z = old_z / BEDROCK_BLOCKS_PER_CHUNK,
			new_column_x = x / BEDROCK_BLOCKS_PER_CHUNK, new_column_z = z / BEDROCK_BLOCKS_PER_CHUNK;

	int8_t c_x, c_y, c_z, new_y, new_p;

	bool update_loc, update_rot, update_chunk;

	bedrock_node *node, *node2;

	old_column_x = (int) (old_column_x >= 0 ? ceil(old_column_x) : floor(old_column_x));
	old_column_z = (int) (old_column_z >= 0 ? ceil(old_column_z) : floor(old_column_z));
	new_column_x = (int) (new_column_x >= 0 ? ceil(new_column_x) : floor(new_column_x));
	new_column_z = (int) (new_column_z >= 0 ? ceil(new_column_z) : floor(new_column_z));

	if (old_x == x && old_y == y && old_z == z && old_yaw == yaw && old_pitch == pitch && old_stance == stance && old_on_ground == on_ground)
		return;
	/* Bursting clients try to move themselves during login for some reason. Don't allow it. */
	else if (client->authenticated == STATE_BURSTING)
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
	if (client->stance != stance)
		client->stance = stance;

	c_x = (x - old_x) * 32;
	c_y = (y - old_y) * 32;
	c_z = (z - old_z) * 32;

	new_y = (yaw / 360.0) * 256;
	new_p = (pitch / 360.0) * 256;

	update_loc = c_x || c_y || c_z;
	update_rot = ((old_yaw / 360.0) * 256) != new_y || ((old_pitch / 360.0) * 256) != new_p;
	update_chunk = old_column_x != new_column_x || old_column_z != new_column_z;

	if (!update_loc && !update_rot)
		return;

	LIST_FOREACH(&client->players, node)
	{
		struct bedrock_client *c = node->data;

		packet_send_entity_teleport(c, client);

		if (update_rot)
			packet_send_entity_head_look(c, client);
	}

	if (update_chunk)
		client_update_chunks(client);

	/* Check if this player should pick up any dropped items near them */
	LIST_FOREACH_SAFE(&client->column->items, node, node2)
	{
		struct bedrock_dropped_item *di = node->data;

		if (abs(x - di->x) <= 1 && abs(y - di->y) <= 1 && abs(z - di->z) <= 1 && client_can_add_inventory_item(client, di->item))
		{
			bedrock_log(LEVEL_DEBUG, "client: %s picks up item %s at %d,%d,%d", client->name, di->item->name, di->x, di->y, di->z);

			packet_send_collect_item(client, di);

			client_add_inventory_item(client, di->item);

			column_free_dropped_item(di);
		}
	}
}

/* Starts the login sequence. This is split up in to two parts because client_update_chunks
 * may not have the chunks available this player is in until later. client_finish_login_sequence
 * is called when the column the player is in is allocated to the client.
 */
void client_start_login_sequence(struct bedrock_client *client)
{
	bedrock_assert(client != NULL && client->authenticated == STATE_BURSTING, return);

	/* Send time */
	packet_send_time(client);

	/* Send world spawn point */
	packet_send_spawn_point(client);

	/* Send chunks */
	client_update_chunks(client);
}

void client_finish_login_sequence(struct bedrock_client *client)
{
	bedrock_node *node;

	bedrock_assert(client != NULL && client->authenticated == STATE_BURSTING, return);

	/* Send player position */
	packet_send_position_and_look(client);

	client->authenticated = STATE_AUTHENTICATED;

	/* Send inventory */
	LIST_FOREACH(&nbt_get(client->data, TAG_LIST, 1, "Inventory")->payload.tag_list, node)
	{
		nbt_tag *c = node->data;
		int16_t *id = nbt_read(c, TAG_SHORT, 1, "id"),
				*damage = nbt_read(c, TAG_SHORT, 1, "Damage");
		int8_t *count = nbt_read(c, TAG_BYTE, 1, "Count"),
				slot;
		struct bedrock_item *item = item_find_or_create(*id);

		nbt_copy(c, TAG_BYTE, &slot, sizeof(slot), 1, "Slot");

		if (slot >= 0 && slot <= 8)
			slot += 36;

		packet_send_set_slot(client, WINDOW_INVENTORY, slot, item, *count, *damage);
	}

	/* Send the player lists */
	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;

		if (c->authenticated == STATE_AUTHENTICATED)
		{
			/* Send this new client to every client that is authenticated */
			packet_send_player_list_item(c, client, true);
			/* Send this client to the new client */
			packet_send_player_list_item(client, c, true);

			packet_send_chat_message(c, "%s joined the game", client->name);
		}
	}

	/* Send nearby players */
	client_update_players(client);
}
