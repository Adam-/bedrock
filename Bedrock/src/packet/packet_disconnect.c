#include "server/client.h"
#include "packet/packet.h"
#include "server/bedrock.h"
#include "io/io.h"

int packet_disconnect(struct bedrock_client *client, const unsigned char *buffer, size_t len)
{
	size_t offset = 1;
	char reason[64];

	packet_read_string(buffer, len, &offset, reason, sizeof(reason));

	bedrock_log(LEVEL_INFO, "client: Received disconnect from %s (%s) - %s", *client->name ? client->name : "(unknown)", client_get_ip(client), reason);

	return offset;
}

void packet_send_disconnect(struct bedrock_client *client, const char *reason)
{
	bedrock_log(LEVEL_INFO, "client: Kicking client %s (%s) - %s", *client->name ? client->name : "(unknown)", client_get_ip(client), reason);

	client_send_header(client, DISCONNECT);
	client_send_string(client, reason);

	io_set(&client->fd, 0, OP_READ);
}
