#ifndef BEDROCK_SERVER_CLIENT_H
#define BEDROCK_SERVER_CLIENT_H

#include "util/fd.h"
#include "util/list.h"
#include "server/config.h"

typedef enum
{
	STATE_UNAUTHENTICATED, /* Not at all authenticated */
	STATE_HANDSHAKING,     /* Doing connection handshake */
	STATE_LOGGING_IN,      /* Logging in, after handshake */
	STATE_AUTHENTICATED
} bedrock_client_authentication_state;

typedef struct
{
	bedrock_fd fd;

	unsigned char in_buffer[1024];
	size_t in_buffer_len;

	unsigned char out_buffer[1024];
	size_t out_buffer_len;

	char name[BEDROCK_USERNAME_MAX];
	char ip[INET6_ADDRSTRLEN];
	bedrock_client_authentication_state authenticated;
} bedrock_client;

extern bedrock_list client_list;

extern bedrock_client *client_create();
extern void client_exit(bedrock_client *client);
extern void client_process_exits();

extern void client_event_read(bedrock_fd *fd, void *data);
extern void client_event_write(bedrock_fd *fd, void *data);

extern const char *client_get_ip(bedrock_client *client);

extern void client_send_header(bedrock_client *client, uint8_t header);
extern void client_send(bedrock_client *client, void *data, size_t size);
extern void client_send_string(bedrock_client *client, const char *string);

#endif // BEDROCK_SERVER_CLIENT_H
