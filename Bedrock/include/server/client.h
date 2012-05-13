#ifndef BEDROCK_SERVER_CLIENT_H
#define BEDROCK_SERVER_CLIENT_H

#include "util/fd.h"
#include "util/list.h"

typedef struct
{
	bedrock_fd fd;

	unsigned char in_buffer[1024];
	size_t in_buffer_len;

	unsigned char out_buffer[1024];
	size_t out_buffer_len;

	char name[128];
	char ip[INET6_ADDRSTRLEN];
} bedrock_client;

extern bedrock_list client_list;

extern bedrock_client *bedrock_client_create();
extern void bedrock_client_exit(bedrock_client *client);
extern void bedrock_client_process_exits();

extern void bedrock_client_event_read(bedrock_fd *fd, void *data);
extern void bedrock_client_event_write(bedrock_fd *fd, void *data);

extern const char *bedrock_client_get_ip(bedrock_client *client);

extern void bedrock_client_send(bedrock_client *client, void *data, size_t size);

#endif // BEDROCK_SERVER_CLIENT_H
