#ifndef BEDROCK_SERVER_CLIENT_H
#define BEDROCK_SERVER_CLIENT_H

#include "util/fd.h"
#include "util/list.h"
#include "util/buffer.h"
#include "util/memory.h"
#include "config/hard.h"
#include "server/world.h"
#include "server/packet.h"
#include "blocks/items.h"
#include "server/oper.h"
#include "windows/window.h"

#ifdef WIN32
#include "windows_time.h"
#endif

#include <openssl/evp.h>

enum client_authentication_state
{
	STATE_UNAUTHENTICATED = 1 << 0,     /* Not at all authenticated */
	STATE_HANDSHAKING     = 1 << 1,     /* Doing connection handshake, including encryption request/response */
	STATE_LOGGING_IN      = 1 << 2,     /* Logging in, checking session.minecraft.net/whatever else, AES encryption is not enabled yet */
	STATE_LOGGED_IN       = 1 << 3,     /* Successfully authenticated, AES encryption is enabled at this point */
	STATE_BURSTING        = 1 << 4,     /* Successfully authenticated, requested spawn, and in the process of being put into the game */
	STATE_IN_GAME         = 1 << 5,     /* In the game, eg logon sequence is complete */
	STATE_ANY             = ~0,         /* Any state */
};

enum client_entity_action
{
	ACTION_NONE,
	ACTION_CROUCH,
	ACTION_UNCROUCH,
	ACTION_LEAVE_BED,
	ACTION_START_SPRINTING,
	ACTION_STOP_SPRINTING
};

struct client
{
	struct bedrock_fd fd;                /* fd for this client */

	EVP_CIPHER_CTX in_cipher_ctx, out_cipher_ctx; /* Crypto contexts for in and out data */

	unsigned char in_buffer[BEDROCK_CLIENT_RECVQ_LENGTH];
	size_t in_buffer_len;

	bedrock_list out_buffer;              /* list of packets to send, these are already encrypted! */

	uint32_t id;                         /* unique entity id, shared across players and NPCs */
	enum client_authentication_state authenticated;


	char name[BEDROCK_USERNAME_MAX];
	char ip[INET6_ADDRSTRLEN];

	double x;
	double y;
	double z;
	float yaw;
	float pitch;
	uint8_t on_ground;

	struct oper *oper;            /* set if this player is an operator */

	nbt_tag *data;                        /* player's .dat file */
	struct world *world;          /* world this player is in */
	struct column *column;        /* column this player is in, can be NULL if it an unloaded column! */

	bedrock_list columns;                 /* columns this player knows about */

	struct timespec ping_time_sent;       /* time keepalive was sent */
	uint32_t ping_id;                     /* ping id sent */
	uint16_t ping;                        /* ping in ms */

	enum client_entity_action action;  /* action the player is doing */

	double stance;                         /* players's stance */

	int16_t selected_slot;                        /* slot the player has selected (weilded item) */
	struct item_stack inventory[INVENTORY_SIZE];  /* player's inventory, weilded items, craft box, hot bar, etc */

	// Data kept while a player is dragging items on a Window
	struct
	{
		struct item_stack stack;
		bedrock_list slots;   /* list of ints... */
	}
	drag_data;

	// Data kept while a player is digging
	struct
	{
		int32_t x;
		uint8_t y;
		int32_t z;
		uint8_t block_id;
		uint16_t item_id;
		struct timespec end;
	}
	digging_data;

	// Data kept while a window (tile entity) is open
	struct
	{
		uint8_t id;
		uint8_t type;

		/* Coords of tile entity */
		int32_t x;
		uint8_t y;
		int32_t z;
	}
	window_data;
};

extern bedrock_list client_list;
extern int authenticated_client_count;

extern struct client *client_create();
extern struct client *client_find(const char *name);
extern bool client_load(struct client *client);
extern void client_save(struct client *client);
extern void client_save_all();
extern void client_exit(struct client *client);
extern void client_process_exits();

extern void client_event_read(evutil_socket_t fd, short events, void *data);
extern void client_event_write(evutil_socket_t fd, short events, void *data);

extern void client_send_packet(struct client *client, bedrock_packet *packet);

extern const char *client_get_ip(struct client *client);

extern bool client_valid_username(const char *name);

extern bool client_can_add_inventory_item(struct client *client, struct item *item);
extern bool client_add_inventory_item(struct client *client, struct item *item);

extern void client_update_columns(struct client *client);
extern void client_update_players(struct client *client);
extern void client_update_position(struct client *client, double x, double y, double z, float yaw, float pitch, double stance, uint8_t on_ground);

extern void client_start_login_sequence(struct client *client);
extern void client_finish_login_sequence(struct client *client);

#endif // BEDROCK_SERVER_CLIENT_H
