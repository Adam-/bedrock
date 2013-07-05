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
#include "packet/packet_entity_properties.h"
#include "packet/packet_keep_alive.h"
#include "packet/packet_position_and_look.h"
#include "packet/packet_player_list_item.h"
#include "packet/packet_spawn_named_entity.h"
#include "packet/packet_spawn_point.h"
#include "packet/packet_set_slot.h"
#include "packet/packet_time.h"
#include "windows/window.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifndef WIN32
#include <arpa/inet.h>
#endif
#include <ctype.h>
#include <math.h>

#define PLAYER_BUFFER_SIZE 4096

bedrock_list client_list = LIST_INIT;
int authenticated_client_count = 0;

static bedrock_list exiting_client_list;

struct client *client_create()
{
	struct client *client = bedrock_malloc(sizeof(struct client));
	EVP_CIPHER_CTX_init(&client->in_cipher_ctx);
	EVP_CIPHER_CTX_init(&client->out_cipher_ctx);
	client->id = ++entity_id;
	client->authenticated = STATE_UNAUTHENTICATED;
	client->out_buffer.free = (bedrock_free_func) bedrock_buffer_free;
	bedrock_list_add(&client_list, client);
	return client;
}

struct client *client_find(const char *name)
{
	bedrock_node *node;

	LIST_FOREACH(&client_list, node)
	{
		struct client *client = node->data;

		if (!strcmp(client->name, name))
			return client;
	}

	return NULL;
}

/* Load data from tag into the given client */
static void client_load_nbt(struct client *client, nbt_tag *tag)
{
	int32_t gamemode;
	nbt_tag *inventory = nbt_get(tag, TAG_LIST, 1, "Inventory");
	if (inventory != NULL)
	{
		bedrock_node *node;

		LIST_FOREACH(&inventory->payload.tag_list.list, node)
		{
			nbt_tag *i = node->data;
			struct item_stack *stack;

			uint16_t *id = nbt_read(i, TAG_SHORT, 1, "id");
			uint16_t *damage = nbt_read(i, TAG_SHORT, 1, "Damage");
			uint8_t *count = nbt_read(i, TAG_BYTE, 1, "Count");
			uint8_t *slot = nbt_read(i, TAG_BYTE, 1, "Slot");

			bedrock_assert(INVENTORY_START + *slot < INVENTORY_SIZE, continue);

			stack = &client->inventory[INVENTORY_START + *slot];

			stack->id = *id;
			stack->count = *count;
			stack->metadata = *damage;
		}

		nbt_free(inventory);
	}

	nbt_copy(tag, TAG_DOUBLE, &client->x, sizeof(client->x), 2, "Pos", 0);
	nbt_copy(tag, TAG_DOUBLE, &client->y, sizeof(client->y), 2, "Pos", 1);
	nbt_copy(tag, TAG_DOUBLE, &client->z, sizeof(client->z), 2, "Pos", 2);
	nbt_copy(tag, TAG_FLOAT, &client->yaw, sizeof(client->yaw), 2, "Rotation", 0);
	nbt_copy(tag, TAG_FLOAT, &client->pitch, sizeof(client->pitch), 2, "Rotation", 1);
	nbt_copy(tag, TAG_BYTE, &client->on_ground, sizeof(client->on_ground), 1, "OnGround");
	nbt_copy(tag, TAG_INT, &gamemode, sizeof(gamemode), 1, "playerGameType");
	client->gamemode = gamemode;

	client->data = tag;
}

bool client_load(struct client *client)
{
	char path[PATH_MAX];
	int fd;
	unsigned char *file_base;
	size_t file_size;
	compression_buffer *cb;
	nbt_tag *tag;

	bedrock_assert(client->world != NULL, return false);

	snprintf(path, sizeof(path), BEDROCK_PLAYER_PATH, client->world->path, client->name);

	fd = open(path, O_RDONLY);
	if (fd == -1)
	{
		bedrock_log(LEVEL_DEBUG, "client: Unable to load player information for %s from %s - %s", client->name, path, strerror(errno));
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

	client_load_nbt(client, tag);

	bedrock_log(LEVEL_DEBUG, "client: Successfully loaded player information file for %s", client->name);

	return true;
}

void client_new(struct client *client)
{
	struct yaml_object *yml;
	nbt_tag *nbt;

	bedrock_assert(client->data == NULL, return);
	bedrock_assert(client->world != NULL, return);

	yml = yaml_parse("data/player.yml");
	if (yml == NULL)
	{
		bedrock_log(LEVEL_WARN, "client: Unable to load player.yml");
		return;
	}

	nbt = nbt_parse_yml(yml);
	bedrock_assert(nbt != NULL, goto error);

	{
		/* Set position from world spawn point */
		int32_t *i_spawn_x = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnX"),
			*i_spawn_y = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnY"),
			*i_spawn_z = nbt_read(client->world->data, TAG_INT, 2, "Data", "SpawnZ");

		double d_spawn_x = *i_spawn_x,
			d_spawn_y = *i_spawn_y,
			d_spawn_z = *i_spawn_z;

		nbt_set(nbt, TAG_DOUBLE, &d_spawn_x, sizeof(d_spawn_x), 2, "Pos", 0);
		nbt_set(nbt, TAG_DOUBLE, &d_spawn_y, sizeof(d_spawn_y), 2, "Pos", 1);
		nbt_set(nbt, TAG_DOUBLE, &d_spawn_z, sizeof(d_spawn_z), 2, "Pos", 2);
	}

	client_load_nbt(client, nbt);

	bedrock_log(LEVEL_DEBUG, "client: Successfully created new player: %s", client->name);

 error:
	yaml_object_free(yml);
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
	
	fd = open(ci->path, O_CREAT | O_WRONLY | O_TRUNC | _O_BINARY, S_IRUSR | S_IWUSR);
	if (fd == -1)
	{
		bedrock_log(LEVEL_WARN, "client: Unable to open player file for %s - %s", ci->name, strerror(errno));
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

/* Convert client data to nbt and return a buffer of it */
static bedrock_buffer *client_save_nbt(struct client *client)
{
	bedrock_buffer *buffer;
	int i;
	int32_t gamemode = client->gamemode;

	/* Build data */
	nbt_tag *inventory = nbt_add(client->data, TAG_LIST, "Inventory", NULL, 0);
	for (i = INVENTORY_START; i < INVENTORY_SIZE; ++i)
	{
		struct item_stack *stack = &client->inventory[i];
		uint8_t b;

		if (!stack->id || !stack->count)
			continue;

		nbt_tag *slot = nbt_add(inventory, TAG_COMPOUND, NULL, NULL, 0);
		nbt_add(slot, TAG_SHORT, "id", &stack->id, sizeof(stack->id));
		nbt_add(slot, TAG_SHORT, "Damage", &stack->metadata, sizeof(stack->metadata));
		nbt_add(slot, TAG_BYTE, "Count", &stack->count, sizeof(stack->count));
		b = i - INVENTORY_START;
		nbt_add(slot, TAG_BYTE, "Slot", &b, sizeof(b));
	}

	nbt_set(client->data, TAG_DOUBLE, &client->x, sizeof(client->x), 2, "Pos", 0);
	nbt_set(client->data, TAG_DOUBLE, &client->y, sizeof(client->y), 2, "Pos", 1);
	nbt_set(client->data, TAG_DOUBLE, &client->z, sizeof(client->z), 2, "Pos", 2);
	nbt_set(client->data, TAG_FLOAT, &client->yaw, sizeof(client->yaw), 2, "Rotation", 0);
	nbt_set(client->data, TAG_FLOAT, &client->pitch, sizeof(client->pitch), 2, "Rotation", 1);
	nbt_set(client->data, TAG_BYTE, &client->on_ground, sizeof(client->on_ground), 1, "OnGround");
	nbt_set(client->data, TAG_INT, &gamemode, sizeof(gamemode), 1, "playerGameType");

	buffer = nbt_write(client->data);

	/* Free data */
	nbt_free(inventory);

	return buffer;
}

void client_save(struct client *client)
{
	struct client_save_info *ci = bedrock_malloc(sizeof(struct client_save_info));

	strncpy(ci->name, client->name, sizeof(ci->name));
	snprintf(ci->path, sizeof(ci->path), BEDROCK_PLAYER_PATH, client->world->path, client->name);
	ci->nbt_out = client_save_nbt(client);

	bedrock_log(LEVEL_DEBUG, "client: Starting save for client %s", client->name);

	bedrock_thread_start((bedrock_thread_entry) client_save_entry, (bedrock_thread_exit) client_save_exit, ci);
}

void client_save_all()
{
	bedrock_node *node;

	LIST_FOREACH(&client_list, node)
	{
		struct client *c = node->data;

		if ((c->authenticated & STATE_IN_GAME) && !(c->authenticated & STATE_BURSTING))
			client_save(c);
	}
}

static void client_free(struct client *client)
{
	bedrock_node *node;

	if (client->authenticated & STATE_IN_GAME)
	{
		if (!(client->authenticated & STATE_BURSTING))
			client_save(client);

		LIST_FOREACH(&client_list, node)
		{
			struct client *c = node->data;

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
			struct client *c = node->data;

			packet_send_destroy_entity_player(c, client);
		}

	LIST_FOREACH(&client->columns, node)
	{
		struct column *c = node->data;

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

	bedrock_fd_close(&client->fd);

	bedrock_list_del(&client_list, client);

	if (client->data != NULL)
		nbt_free(client->data);

	EVP_CIPHER_CTX_cleanup(&client->in_cipher_ctx);
	EVP_CIPHER_CTX_cleanup(&client->out_cipher_ctx);

	bedrock_list_clear(&client->drag_data.slots);
	bedrock_list_clear(&client->out_buffer);
	bedrock_free(client);
}

void client_exit(struct client *client)
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
		struct client *client = n->data;

		client_free(client);
	}

	bedrock_list_clear(&exiting_client_list);
}

void client_event_read(evutil_socket_t fd, short bedrock_attribute_unused events, void *data)
{
	struct client *client = data;
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
	struct client *client = data;
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

void client_send_packet(struct client *client, bedrock_packet *packet)
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

const char *client_get_ip(struct client *client)
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

bool client_can_add_inventory_item(struct client *client, struct item *item)
{
	int i;

	for (i = INVENTORY_START; i < INVENTORY_SIZE; ++i)
	{
		struct item_stack *stack = &client->inventory[i];

		if (stack->count == 0)
			return true; // Empty slot, so it can go here
		else if (item->id == stack->id && stack->count < BEDROCK_MAX_ITEMS_PER_STACK)
			return true; // Item of the same type, can go here too
	}

	return false;
}

bool client_add_inventory_item(struct client *client, struct item *item)
{
	int empty_pos = -1;
	struct item_stack *empty = NULL;
	int i;

	bedrock_log(LEVEL_DEBUG, "client: Adding item %s to %s's inventory", item->name, client->name);

	/* First find if we have another stack of this we can add to */
	for (i = INVENTORY_START; i < INVENTORY_SIZE; ++i)
	{
		struct item_stack *stack = &client->inventory[i];

		if (stack->count == 0)
		{
			/* Slot is empty */
			if (empty == NULL)
			{
				empty_pos = i;
				empty = stack;
			}
			continue;
		}

		if (item->id == stack->id && stack->count < BEDROCK_MAX_ITEMS_PER_STACK)
		{
			++stack->count;

			packet_send_set_slot(client, WINDOW_INVENTORY, i, item, stack->count, stack->metadata);

			return true;
		}
	}

	if (empty == NULL)
		return false; // No empty slots
	
	empty->id = item->id;
	empty->count = 1;
	empty->metadata = 0; // XXX?

	packet_send_set_slot(client, WINDOW_INVENTORY, empty_pos, item, empty->count, empty->metadata);

	return true;
}

static void client_update_column(struct client *client, packet_column_bulk *columns, struct column *column)
{
	bedrock_log(LEVEL_COLUMN, "client: Allocating column %d, %d for %s", column->x, column->z, client->name);

	packet_column_bulk_add(client, columns, column);

	bedrock_list_add(&client->columns, column);
	bedrock_list_add(&column->players, client);
}

void client_update_columns(struct client *client)
{
	/* Update the column around the player. Used for when the player moves to a new column. */
	int i;
	struct region *region;
	struct column *c;
	bedrock_node *node, *node2;
	/* Player coords */
	double player_x = client->x / BEDROCK_BLOCKS_PER_CHUNK, player_z = client->z / BEDROCK_BLOCKS_PER_CHUNK;
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
				column_set_pending(c, COLUMN_FLAG_EMPTY);
		}
	}

	/* Region the player is in */
	region = find_region_which_contains(client->world, client->x, client->z);
	bedrock_assert(region != NULL, return);

	client->column = c = find_column_which_contains(region, client->x, client->z);

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
			region = find_region_which_contains(client->world, client->x + (j * BEDROCK_BLOCKS_PER_CHUNK), client->z + (i * BEDROCK_BLOCKS_PER_CHUNK));
			bedrock_assert(region != NULL, continue);
			c = find_column_which_contains(region, client->x + (j * BEDROCK_BLOCKS_PER_CHUNK), client->z + (i * BEDROCK_BLOCKS_PER_CHUNK));

			if (c == NULL || bedrock_list_has_data(&client->columns, c))
				continue;

			client_update_column(client, &columns, c);
		}

		/* Next, go from i,i to i,-i (exclusive) */
		for (j = i; j > -i; --j)
		{
			region = find_region_which_contains(client->world, client->x + (i * BEDROCK_BLOCKS_PER_CHUNK), client->z + (j * BEDROCK_BLOCKS_PER_CHUNK));
			bedrock_assert(region != NULL, continue);
			c = find_column_which_contains(region, client->x + (i * BEDROCK_BLOCKS_PER_CHUNK), client->z + (j * BEDROCK_BLOCKS_PER_CHUNK));

			if (c == NULL || bedrock_list_has_data(&client->columns, c))
				continue;

			client_update_column(client, &columns, c);
		}

		/* Next, go from i,-i to -i,-i (exclusive) */
		for (j = i; j > -i; --j)
		{
			region = find_region_which_contains(client->world, client->x + (j * BEDROCK_BLOCKS_PER_CHUNK), client->z - (i * BEDROCK_BLOCKS_PER_CHUNK));
			bedrock_assert(region != NULL, continue);
			c = find_column_which_contains(region, client->x + (j * BEDROCK_BLOCKS_PER_CHUNK), client->z - (i * BEDROCK_BLOCKS_PER_CHUNK));

			if (c == NULL || bedrock_list_has_data(&client->columns, c))
				continue;

			client_update_column(client, &columns, c);
		}

		/* Next, go from -i,-i to -i,i (exclusive) */
		for (j = -i; j < i; ++j)
		{
			region = find_region_which_contains(client->world, client->x - (i * BEDROCK_BLOCKS_PER_CHUNK), client->z + (j * BEDROCK_BLOCKS_PER_CHUNK));
			bedrock_assert(region != NULL, continue);
			c = find_column_which_contains(region, client->x - (i * BEDROCK_BLOCKS_PER_CHUNK), client->z + (j * BEDROCK_BLOCKS_PER_CHUNK));

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
void client_update_position(struct client *client, double x, double y, double z, float yaw, float pitch, double stance, uint8_t on_ground)
{
	double old_x = client->x, old_y = client->y, old_z = client->z;
	float old_yaw = client->yaw, old_pitch = client->pitch;
	double old_stance = client->stance;
	uint8_t old_on_ground = client->on_ground;

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
		client->x = x;
	if (old_y != y)
		client->y = y;
	if (old_z != z)
		client->z = z;
	if (old_yaw != yaw)
		client->yaw = yaw;
	if (old_pitch != pitch)
		client->pitch = pitch;
	if (client->stance != stance)
		client->stance = stance;
	if (client->on_ground != on_ground)
		client->on_ground = on_ground;

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
			struct client *c = node->data;

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
			struct dropped_item *di = node->data;

			if (abs(x - di->x) <= 1 && abs(y - di->y) <= 1 && abs(z - di->z) <= 1 && client_can_add_inventory_item(client, di->item))
			{
				while (di->count && client_add_inventory_item(client, di->item))
					--di->count;

				if (!di->count)
				{
					bedrock_log(LEVEL_DEBUG, "client: %s picks up item %s at %d,%d,%d", client->name, di->item->name, di->x, di->y, di->z);
					packet_send_collect_item(client, di);
					column_free_dropped_item(di);
				}
				else
				{
					bedrock_log(LEVEL_DEBUG, "client: %s picks up all but %d item %s at %d,%d,%d", client->name, di->count, di->item->name, di->x, di->y, di->z);
				}
			}
		}
}

/* Starts the login sequence. This is split up in to two parts because client_update_columns
 * may not have the chunks available this player is in until later. client_finish_login_sequence
 * is called when the column the player is in is allocated to the client.
 */
void client_start_login_sequence(struct client *client)
{
	bedrock_assert(client != NULL && client->authenticated == STATE_BURSTING, return);

	/* Send time */
	packet_send_time(client);

	/* Send world spawn point */
	packet_send_spawn_point(client);

	/* Send chunks */
	client_update_columns(client);
}

void client_finish_login_sequence(struct client *client)
{
	bedrock_node *node;
	struct oper *oper;
	int i;

	bedrock_assert(client != NULL && client->authenticated == STATE_BURSTING, return);

	/* Send player position */
	packet_send_position_and_look(client);

	/* Send inventory */
	for (i = INVENTORY_START; i < INVENTORY_SIZE; ++i)
	{
		struct item_stack *stack = &client->inventory[i];

		if (stack->count == 0)
			continue;

		struct item *item = item_find_or_create(stack->id);

		packet_send_set_slot(client, WINDOW_INVENTORY, i, item, stack->count, stack->metadata);
	}

	/* Send the player lists */
	LIST_FOREACH(&client_list, node)
	{
		struct client *c = node->data;

		if (c == client || c->authenticated & STATE_IN_GAME)
		{
			/* Send this new client to every client that is authenticated */
			packet_send_player_list_item(c, client, true);
			/* Send this client to the new client */
			packet_send_player_list_item(client, c, true);

			packet_send_chat_message(c, "%s joined the game", client->name);
		}
	}

	packet_send_entity_properties(client, "generic.movementSpeed", BEDROCK_DEFAUT_PLAYER_WALK_SPEED);

	/* Once this comes back we know the client is synced */
	packet_send_keep_alive(client, ~0);

	client->authenticated |= STATE_IN_GAME;

	oper = oper_find(client->name);
	if (oper != NULL && *oper->password == 0)
		client->oper = oper;
}
