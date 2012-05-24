#ifndef BEDROCK_SERVER_CLIENT_H
#define BEDROCK_SERVER_CLIENT_H

#include "util/fd.h"
#include "util/list.h"
#include "server/config.h"
#include "server/world.h"
#include "util/buffer.h"

typedef enum
{
	STATE_UNAUTHENTICATED = 1 << 0,     /* Not at all authenticated */
	STATE_HANDSHAKING     = 1 << 1,     /* Doing connection handshake */
	//STATE_LOGGING_IN      = 1 << 2,     /* Logging in, after handshake */
	STATE_AUTHENTICATED   = 1 << 3,     /* Authenticated and in the game */
	STATE_ANY             = ~0,         /* Any state */
} bedrock_client_authentication_state;

struct bedrock_client
{
	bedrock_fd fd;

	uint32_t id;
	bedrock_client_authentication_state authenticated;

	unsigned char in_buffer[BEDROCK_CLIENT_RECVQ_LENGTH];
	size_t in_buffer_len;

	bedrock_buffer *out_buffer;

	char name[BEDROCK_USERNAME_MAX];
	char ip[INET6_ADDRSTRLEN];

	nbt_tag *data;                  /* player's .dat file */
	struct bedrock_world *world;    /* world this player is in */

	bedrock_list columns;           /* columns this player knows about */
	bedrock_list players;           /* players this player knows about */
};

extern bedrock_list client_list;
extern uint32_t entity_id;

extern struct bedrock_client *client_create();
extern struct bedrock_client *client_find(const char *name);
extern bool client_load(struct bedrock_client *client);
extern void client_exit(struct bedrock_client *client);
extern void client_process_exits();

extern void client_event_read(bedrock_fd *fd, void *data);
extern void client_event_write(bedrock_fd *fd, void *data);

extern const char *client_get_ip(struct bedrock_client *client);

extern void client_send_header(struct bedrock_client *client, uint8_t header);
extern void client_send(struct bedrock_client *client, const void *data, size_t size);
extern void client_send_int(struct bedrock_client *client, const void *data, size_t size);
extern void client_send_string(struct bedrock_client *client, const char *string);

extern bool client_valid_username(const char *name);

extern double *client_get_pos_x(struct bedrock_client *client);
extern double *client_get_pos_y(struct bedrock_client *client);
extern double *client_get_pos_z(struct bedrock_client *client);
extern float *client_get_yaw(struct bedrock_client *client);
extern float *client_get_pitch(struct bedrock_client *client);
extern uint8_t *client_get_on_ground(struct bedrock_client *client);

extern void client_update_chunks(struct bedrock_client *client);
extern void client_update_players(struct bedrock_client *client);
extern void client_update_position(struct bedrock_client *client, double x, double y, double z, float yaw, float pitch, uint8_t on_ground);

extern void client_send_login_sequence(struct bedrock_client *client);

#endif // BEDROCK_SERVER_CLIENT_H
