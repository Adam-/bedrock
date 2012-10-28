#include "server/bedrock.h"
#include "server/client.h"
#include "server/column.h"
#include "server/packet.h"
#include "util/compression.h"
#include "util/crypto.h"
#include "util/endian.h"
#include "util/file.h"
#include "util/io.h"
#include "nbt/nbt.h"
#include "packet/packet_chat_message.h"
#include "packet/packet_collect_item.h"
#include "packet/packet_column.h"
#include "packet/packet_column_bulk.h"
#include "packet/packet_destroy_entity.h"
#include "packet/packet_entity_head_look.h"
#include "packet/packet_entity_teleport.h"
#include "packet/packet_keep_alive.h"
#include "packet/packet_position_and_look.h"
#include "packet/packet_player_list_item.h"
#include "packet/packet_spawn_named_entity.h"
#include "packet/packet_spawn_point.h"
#include "packet/packet_set_slot.h"
#include "packet/packet_time.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <math.h>

#define PLAYER_BUFFER_SIZE 4096

bedrock_list client_list = LIST_INIT;
int authenticated_client_count = 0;

static bedrock_list exiting_client_list;

struct bedrock_client *client_create()
{
	struct bedrock_client *client = bedrock_malloc(sizeof(struct bedrock_client));
	EVP_CIPHER_CTX_init(&client->in_cipher_ctx);
	EVP_CIPHER_CTX_init(&client->out_cipher_ctx);
	client->id = ++entity_id;
	client->authenticated = STATE_UNAUTHENTICATED;
	client->out_buffer.free = (bedrock_free_func) bedrock_buffer_free;
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
	unsigned char *file_base;
	size_t file_size;
	compression_buffer *cb;
	nbt_tag *tag;

	bedrock_assert(client != NULL && client->world != NULL, return false);

	snprintf(path, sizeof(path), BEDROCK_PLAYER_PATH, client->world->path, client->name);

	fd = open(path, O_RDONLY);
	if (fd == -1)
	{
		bedrock_log(LEVEL_CRIT, "client: Unable to load player information for %s from %s - %s", client->name, path, strerror(errno));
		return false;
	}

	file_base = bedrock_file_read(fd, &file_size);
	if (file_base == NULL)
	{
		bedrock_log(LEVEL_CRIT, "client: Unable to read player information file %s", path);
		close(fd);
		bedrock_free(file_base);
		return false;
	}

	close(fd);

	cb = compression_decompress(PLAYER_BUFFER_SIZE, file_base, file_size);
	bedrock_free(file_base);
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

struct client_save_info
{
	char name[BEDROCK_USERNAME_MAX];
	char path[PATH_MAX];
	bedrock_buffer *nbt_out;
};

static void client_save_entry(struct bedrock_thread bedrock_attribute_unused *thread, struct client_save_info *ci)
{
	int fd;
	compression_buffer *buffer;
	
	fd = open(ci->path, O_WRONLY | O_TRUNC | _O_BINARY);
	if (fd == -1)
	{
		bedrock_log(LEVEL_WARN, "client: Unable to open client file for %s - %s", ci->name, strerror(errno));
		return;
	}

	buffer = compression_compress(PLAYER_BUFFER_SIZE, ci->nbt_out->data, ci->nbt_out->length);

	if (write(fd, buffer->buffer->data, buffer->buffer->length) != (int) buffer->buffer->length)
	{
		bedrock_log(LEVEL_WARN, "client: Unable to save data for client %s - %s", ci->name, strerror(errno));
	}

	compression_compress_end(buffer);

	close(fd);
}

static void client_save_exit(struct client_save_info *ci)
{
	bedrock_log(LEVEL_DEBUG, "client: Finished saving for client %s", ci->name);

	bedrock_buffer_free(ci->nbt_out);
	bedrock_free(ci);
}

void client_save(struct bedrock_client *client)
{
	struct client_save_info *ci = bedrock_malloc(sizeof(struct client_save_info));

	strncpy(ci->name, client->name, sizeof(ci->name));
	snprintf(ci->path, sizeof(ci->path), BEDROCK_PLAYER_PATH, client->world->path, client->name);
	ci->nbt_out = nbt_write(client->data);

	bedrock_log(LEVEL_DEBUG, "client: Starting save for client %s", client->name);

	bedrock_thread_start((bedrock_thread_entry) client_save_entry, (bedrock_thread_exit) client_save_exit, ci);
}

void client_save_all()
{
	bedrock_node *node;

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;

		if ((c->authenticated & STATE_IN_GAME) && !(c->authenticated & STATE_BURSTING))
			client_save(c);
	}
}

static void client_free(struct bedrock_client *client)
{
	bedrock_node *node;

	if (client->authenticated & STATE_IN_GAME)
	{
		if (!(client->authenticated & STATE_BURSTING))
			client_save(client);

		LIST_FOREACH(&client_list, node)
		{
			struct bedrock_client *c = node->data;

			if (c->authenticated & STATE_IN_GAME && c != client)
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

	if (client->column)
		LIST_FOREACH(&client->column->players, node)
		{
			struct bedrock_client *c = node->data;

			packet_send_destroy_entity_player(c, client);
		}

	LIST_FOREACH(&client->columns, node)
	{
		struct bedrock_column *c = node->data;

		bedrock_list_del(&c->players, client);

		/* Column is no longer in render distance of anything */
		if (c->players.count == 0)
		{
			/* No operations are pending on this column, delete it immediately */
			if (c->flags == 0)
				column_free(c);
			else
				/* Mark it as want free */
				column_set_pending(c, COLUMN_FLAG_EMPTY);
		}
	}
	bedrock_list_clear(&client->columns);

	bedrock_log(LEVEL_DEBUG, "client: Exiting client %s from %s", *client->name ? client->name : "(unknown)", client_get_ip(client));

	io_disable(&client->fd.event_read);
	io_disable(&client->fd.event_write);

	bedrock_fd_close(&client->fd);

	bedrock_list_del(&client_list, client);

	if (client->data != NULL)
		nbt_free(client->data);

	EVP_CIPHER_CTX_cleanup(&client->in_cipher_ctx);
	EVP_CIPHER_CTX_cleanup(&client->out_cipher_ctx);

	bedrock_list_clear(&client->out_buffer);
	bedrock_free(client);
}

void client_exit(struct bedrock_client *client)
{
	if (bedrock_list_has_data(&exiting_client_list, client) == false)
	{
		bedrock_list_add(&exiting_client_list, client);
		io_disable(&client->fd.event_read);
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

void client_event_read(evutil_socket_t fd, short bedrock_attribute_unused events, void *data)
{
	struct bedrock_client *client = data;
	bedrock_packet packet;
	unsigned char buffer[BEDROCK_CLIENT_RECVQ_LENGTH];
	int i;

	if (client->in_buffer_len == sizeof(client->in_buffer))
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Receive queue exceeded for %s (%s) - dropping client", *client->name ? client->name : "(unknown)", client_get_ip(client));
		client_exit(client);
		return;
	}

	bedrock_assert(sizeof(buffer) == sizeof(client->in_buffer), ;);
	i = recv(fd, buffer, sizeof(client->in_buffer) - client->in_buffer_len, 0);
	if (i <= 0)
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Lost connection from client %s (%s)", *client->name ? client->name : "(unknown)", client_get_ip(client));

		io_disable(&client->fd.event_write);

		client_exit(client);
		return;
	}

	if (client->authenticated >= STATE_LOGGED_IN)
	{
		bedrock_assert(i == crypto_aes_decrypt(&client->in_cipher_ctx, buffer, i, client->in_buffer + client->in_buffer_len, sizeof(client->in_buffer) - client->in_buffer_len), ;);
	}
	else
		memcpy(client->in_buffer + client->in_buffer_len, buffer, i);

	client->in_buffer_len += i;

	packet.data = client->in_buffer;
	packet.length = client->in_buffer_len;
	packet.capacity = sizeof(client->in_buffer);

	while (io_is_pending(&client->fd.event_read, EV_READ) && (i = packet_parse(client, &packet)) > 0)
	{
		bedrock_assert((size_t) i <= client->in_buffer_len, break);

		packet.length = client->in_buffer_len -= i;

		if (client->in_buffer_len > 0)
			memmove(client->in_buffer, client->in_buffer + i, client->in_buffer_len);
	}
}

void client_event_write(evutil_socket_t fd, short bedrock_attribute_unused events, void *data)
{
	struct bedrock_client *client = data;
	bedrock_node *node;
	bedrock_packet *packet;
	int i;

	if (client->out_buffer.count == 0)
	{
		io_disable(&client->fd.event_write);

		if (io_is_pending(&client->fd.event_read, EV_READ) == false)
			client_exit(client);
		return;
	}

	node = client->out_buffer.head;
	packet = node->data;

	i = send(fd, packet->data, packet->length, 0);
	if (i <= 0)
	{
		if (bedrock_list_has_data(&exiting_client_list, client) == false)
			bedrock_log(LEVEL_INFO, "Lost connection from client %s (%s)", *client->name ? client->name : "(unknown)", client_get_ip(client));

		io_disable(&client->fd.event_write);

		client_exit(client);
		return;
	}

	packet->length -= i;
	if (packet->length > 0)
	{
		memmove(packet->data, packet->data + i, packet->length);
	}
	else
	{
		bedrock_list_del_node(&client->out_buffer, node);
		bedrock_free(node);

		if (client->out_buffer.count == 0)
		{
			io_disable(&client->fd.event_write);

			if (io_is_pending(&client->fd.event_read, EV_READ) == false)
				client_exit(client);
		}
	}
}

void client_send_packet(struct bedrock_client *client, bedrock_packet *packet)
{
	bedrock_packet *p;
	struct packet_info *pi;

	bedrock_assert(client != NULL && packet != NULL && packet->length > 0, return);

	pi = packet_find(*packet->data);
	bedrock_assert(pi != NULL, bedrock_log(LEVEL_PACKET_DEBUG, "packet: Sending unknown packet 0x%02x", *packet->data));
	if (pi != NULL)
	{
		bedrock_assert((pi->flags & SOFT_SIZE) ? packet->length >= pi->len : packet->length == pi->len, ;);
		bedrock_assert((pi->flags & CLIENT_ONLY) == 0, ;);
	}
	bedrock_log(LEVEL_PACKET_DEBUG, "packet: Queueing packet header 0x%02x for %s (%s)", *packet->data, *client->name ? client->name : "(unknown)", client_get_ip(client));

	p = bedrock_malloc(sizeof(bedrock_packet));

	strncpy(p->name, packet->name, sizeof(p->name));

	if (client->authenticated >= STATE_LOGGED_IN)
	{
		bedrock_buffer_ensure_capacity(p, packet->capacity + EVP_CIPHER_CTX_block_size(&client->out_cipher_ctx));
		p->length = crypto_aes_encrypt(&client->out_cipher_ctx, packet->data, packet->length, p->data, p->capacity);

		bedrock_free(packet->data);
		packet->data = NULL;
		packet->length = 0;
		packet->capacity = 0;
	}
	else
	{
		p->data = packet->data;
		p->length = packet->length;
		p->capacity = packet->capacity;

		packet->data = NULL;
		packet->length = 0;
		packet->capacity = 0;
	}

	bedrock_list_add(&client->out_buffer, p);

	io_enable(&client->fd.event_write);
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

	LIST_FOREACH(&nbt_get(client->data, TAG_LIST, 1, "Inventory")->payload.tag_list.list, node)
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

	LIST_FOREACH(&nbt_get(client->data, TAG_LIST, 1, "Inventory")->payload.tag_list.list, node)
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
	nbt_tag *inven = nbt_get(client->data, TAG_LIST, 1, "Inventory");
	struct nbt_tag_list *tag_list = &inven->payload.tag_list;
	bedrock_list *list = &tag_list->list;
	bedrock_node *node;
	int i = -1;
	nbt_tag *c, *item_tag;
	uint8_t pos;

	bedrock_log(LEVEL_DEBUG, "client: Adding item %s to %s's inventory", item->name, client->name);

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
			if (pos <= 8)
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
	if (pos <= 8)
		pos += 36;
	packet_send_set_slot(client, WINDOW_INVENTORY, pos, item, 1, 0);

	item_tag = bedrock_malloc(sizeof(nbt_tag));
	item_tag->owner = inven;
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
			bedrock_list_add_node_before(list, bedrock_malloc(sizeof(bedrock_node)), node, item_tag);
			++tag_list->length;
			return;
		}
	}

	// Insert at the end
	bedrock_list_add(list, item_tag);
	++tag_list->length;
}

static void client_update_column(struct bedrock_client *client, packet_column_bulk *columns, struct bedrock_column *column)
{
	bedrock_log(LEVEL_COLUMN, "client: Allocating column %d, %d for %s", column->x, column->z, client->name);

	packet_column_bulk_add(client, columns, column);

	bedrock_list_add(&client->columns, column);
	bedrock_list_add(&column->players, client);
}

void client_update_columns(struct bedrock_client *client)
{
	/* Update the column around the player. Used for when the player moves to a new column. */
	int i;
	struct bedrock_region *region;
	struct bedrock_column *c;
	bedrock_node *node, *node2;
	/* Player coords */
	double x = *client_get_pos_x(client), z = *client_get_pos_z(client);
	double player_x = x / BEDROCK_BLOCKS_PER_CHUNK, player_z = z / BEDROCK_BLOCKS_PER_CHUNK;
	packet_column_bulk columns = PACKET_COLUMN_BULK_INIT;
	bool finish = false;

	player_x = (int) (player_x >= 0 ? ceil(player_x) : floor(player_x));
	player_z = (int) (player_z >= 0 ? ceil(player_z) : floor(player_z));

	/* Find columns we can delete */
	LIST_FOREACH_SAFE(&client->columns, node, node2)
	{
		c = node->data;

		if (abs(c->x - player_x) > BEDROCK_VIEW_LENGTH || abs(c->z - player_z) > BEDROCK_VIEW_LENGTH)
		{
			bedrock_list_del_node(&client->columns, node);
			bedrock_free(node);

			bedrock_list_del(&c->players, client);

			bedrock_log(LEVEL_COLUMN, "client: Unallocating column %d, %d for %s", c->x, c->z, client->name);

			packet_send_column_empty(client, c);

			/* Column is no longer in render distance of anything */
			if (c->players.count == 0)
			{
				/* No operations are pending on this column, delete it immediately */
				if (c->flags == 0)
					column_free(c);
				else
					/* Mark it as want free */
					column_set_pending(c, COLUMN_FLAG_EMPTY);
			}
		}
	}

	/* Region the player is in */
	region = find_region_which_contains(client->world, x, z);
	bedrock_assert(region != NULL, return);

	client->column = c = find_column_which_contains(region, x, z);

	if (c != NULL)
		if (bedrock_list_has_data(&client->columns, c) == false)
		{
			client_update_column(client, &columns, c);

			/* Loading the column the player is in on a bursting player, finish burst */
			if (client->authenticated == STATE_BURSTING)
				finish = true;
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

			client_update_column(client, &columns, c);
		}

		/* Next, go from i,i to i,-i (exclusive) */
		for (j = i; j > -i; --j)
		{
			region = find_region_which_contains(client->world, x + (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));
			bedrock_assert(region != NULL, continue);
			c = find_column_which_contains(region, x + (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));

			if (c == NULL || bedrock_list_has_data(&client->columns, c))
				continue;

			client_update_column(client, &columns, c);
		}

		/* Next, go from i,-i to -i,-i (exclusive) */
		for (j = i; j > -i; --j)
		{
			region = find_region_which_contains(client->world, x + (j * BEDROCK_BLOCKS_PER_CHUNK), z - (i * BEDROCK_BLOCKS_PER_CHUNK));
			bedrock_assert(region != NULL, continue);
			c = find_column_which_contains(region, x + (j * BEDROCK_BLOCKS_PER_CHUNK), z - (i * BEDROCK_BLOCKS_PER_CHUNK));

			if (c == NULL || bedrock_list_has_data(&client->columns, c))
				continue;

			client_update_column(client, &columns, c);
		}

		/* Next, go from -i,-i to -i,i (exclusive) */
		for (j = -i; j < i; ++j)
		{
			region = find_region_which_contains(client->world, x - (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));
			bedrock_assert(region != NULL, continue);
			c = find_column_which_contains(region, x - (i * BEDROCK_BLOCKS_PER_CHUNK), z + (j * BEDROCK_BLOCKS_PER_CHUNK));

			if (c == NULL || bedrock_list_has_data(&client->columns, c))
				continue;

			client_update_column(client, &columns, c);
		}
	}

	if (columns.count > 0)
		packet_send_column_bulk(client, &columns);

	if (finish)
		client_finish_login_sequence(client);
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

	bool update_loc, update_rot, update_col;

	bedrock_node *node, *node2;

	old_column_x = (int) (old_column_x >= 0 ? ceil(old_column_x) : floor(old_column_x));
	old_column_z = (int) (old_column_z >= 0 ? ceil(old_column_z) : floor(old_column_z));
	new_column_x = (int) (new_column_x >= 0 ? ceil(new_column_x) : floor(new_column_x));
	new_column_z = (int) (new_column_z >= 0 ? ceil(new_column_z) : floor(new_column_z));

	if (old_x == x && old_y == y && old_z == z && old_yaw == yaw && old_pitch == pitch && old_stance == stance && old_on_ground == on_ground)
		return;
	/* Don't allow bursting clients to move around */
	else if (client->authenticated & STATE_BURSTING)
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
	update_col = old_column_x != new_column_x || old_column_z != new_column_z;

	if (!update_loc && !update_rot)
		return;

	if (client->column != NULL)
		LIST_FOREACH(&client->column->players, node)
		{
			struct bedrock_client *c = node->data;

			if (c == client)
				continue;

			packet_send_entity_teleport(c, client);

			if (update_rot)
				packet_send_entity_head_look(c, client);
		}

	if (update_col)
		client_update_columns(client);

	if (client->column != NULL)
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

/* Starts the login sequence. This is split up in to two parts because client_update_columns
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
	client_update_columns(client);
}

void client_finish_login_sequence(struct bedrock_client *client)
{
	bedrock_node *node;
	struct bedrock_oper *oper;

	bedrock_assert(client != NULL && client->authenticated == STATE_BURSTING, return);

	/* Send player position */
	packet_send_position_and_look(client);

	/* Send inventory */
	LIST_FOREACH(&nbt_get(client->data, TAG_LIST, 1, "Inventory")->payload.tag_list.list, node)
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

		if (c == client || c->authenticated & STATE_IN_GAME)
		{
			/* Send this new client to every client that is authenticated */
			packet_send_player_list_item(c, client, true);
			/* Send this client to the new client */
			packet_send_player_list_item(client, c, true);

			packet_send_chat_message(c, "%s joined the game", client->name);
		}
	}

	/* Once this comes back we know the client is synced */
	packet_send_keep_alive(client, ~0);

	client->authenticated |= STATE_IN_GAME;

	oper = oper_find(client->name);
	if (oper != NULL && *oper->password == 0)
		client->oper = oper;
}
