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

typedef enum
{
	STATE_UNAUTHENTICATED = 1 << 0,     /* Not at all authenticated */
	STATE_HANDSHAKING     = 1 << 1,     /* Doing connection handshake */
	//STATE_LOGGING_IN      = 1 << 2,     /* Logging in, after handshake */
	STATE_BURSTING        = 1 << 3,     /* Successfully authenticated but not in the game yet */
	STATE_AUTHENTICATED   = 1 << 4,     /* Authenticated and in the game */
	STATE_ANY             = ~0,         /* Any state */
} bedrock_client_authentication_state;

typedef enum
{
	ACTION_NONE,
	ACTION_CROUCH,
	ACTION_UNCROUCH,
	ACTION_LEAVE_BED,
	ACTION_START_SPRINTING,
	ACTION_STOP_SPRINTING
} bedrock_client_entity_action;

struct bedrock_client
{
	struct bedrock_fd fd;                       /* fd for this client */

	uint32_t id;                         /* unique entity id, shared across players and NPCs */
	bedrock_client_authentication_state authenticated;

	unsigned char in_buffer[BEDROCK_CLIENT_RECVQ_LENGTH];
	size_t in_buffer_len;

	bedrock_list out_buffer;              /* list of packets to send */

	char name[BEDROCK_USERNAME_MAX];
	char ip[INET6_ADDRSTRLEN];

	struct bedrock_oper *oper;            /* set if this player is an operator */

	nbt_tag *data;                        /* player's .dat file */
	struct bedrock_world *world;          /* world this player is in */
	struct bedrock_column *column;        /* column this player is in, can be NULL if it an unloaded column! */

	bedrock_list columns;                 /* columns this player knows about */

	struct timespec ping_time_sent;       /* time keepalive was sent */
	uint32_t ping_id;                     /* ping id sent */
	uint16_t ping;                        /* ping in ms */

	int16_t selected_slot;                /* slot the player has selected (weilded item), 0-8 */
	bedrock_client_entity_action action;  /* action the player is doing */

	double stance;                         /* players's stance */

	// Data kept while a player is dragging items on a Window
	struct
	{
		int16_t id;
		uint8_t count;
		int16_t metadata;
	}
	window_drag_data;

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
};

extern bedrock_list client_list;
extern int authenticated_client_count;

extern struct bedrock_client *client_create();
extern struct bedrock_client *client_find(const char *name);
extern bool client_load(struct bedrock_client *client);
extern void client_exit(struct bedrock_client *client);
extern void client_process_exits();

extern void client_event_read(struct bedrock_fd *fd, void *data);
extern void client_event_write(struct bedrock_fd *fd, void *data);

extern void client_send_packet(struct bedrock_client *client, bedrock_packet *packet);

extern const char *client_get_ip(struct bedrock_client *client);

extern bool client_valid_username(const char *name);

extern double *client_get_pos_x(struct bedrock_client *client);
extern double *client_get_pos_y(struct bedrock_client *client);
extern double *client_get_pos_z(struct bedrock_client *client);
extern float *client_get_yaw(struct bedrock_client *client);
extern float *client_get_pitch(struct bedrock_client *client);
extern uint8_t *client_get_on_ground(struct bedrock_client *client);

extern nbt_tag *client_get_inventory_tag(struct bedrock_client *client, uint8_t slot);
extern bool client_can_add_inventory_item(struct bedrock_client *client, struct bedrock_item *item);
extern void client_add_inventory_item(struct bedrock_client *client, struct bedrock_item *item);

extern void client_update_columns(struct bedrock_client *client);
extern void client_update_players(struct bedrock_client *client);
extern void client_update_position(struct bedrock_client *client, double x, double y, double z, float yaw, float pitch, double stance, uint8_t on_ground);

extern void client_start_login_sequence(struct bedrock_client *client);
extern void client_finish_login_sequence(struct bedrock_client *client);

#endif // BEDROCK_SERVER_CLIENT_H
