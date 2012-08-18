#ifndef BEDROCK_PROTOCOL_CONSOLE_H
#define BEDROCK_PROTOCOL_CONSOLE_H


#include "util/list.h"
#include "util/fd.h"
#include "config/hard.h"

extern bedrock_list console_list;

struct bedrock_console_client
{
	struct bedrock_fd fd;	/* Client FD */

	unsigned char in_buffer[BEDROCK_CLIENT_RECVQ_LENGTH];
	size_t in_buffer_len;

	bedrock_list out_buffer;	/* List of data to send, plain ASCII text */
};

extern void console_init();
extern void console_shutdown();
extern struct bedrock_console_client *console_client_create();
extern void console_exit(struct bedrock_console_client *client);
extern void console_process_exits();
extern void console_write(struct bedrock_console_client *client, const char *string);

#endif // BEDROCK_PROTOCOL_CONSOLE_H
