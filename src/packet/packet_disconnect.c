#include "server/client.h"
#include "server/packet.h"
#include "server/bedrock.h"
#include "io/io.h"

int packet_disconnect(struct bedrock_client *client, const bedrock_packet *p)
{
	size_t offset = PACKET_HEADER_LENGTH;
	char reason[BEDROCK_MAX_STRING_LENGTH];

	packet_read_string(p, &offset, reason, sizeof(reason));

	bedrock_log(LEVEL_INFO, "client: Received disconnect from %s (%s) - %s", *client->name ? client->name : "(unknown)", client_get_ip(client), *reason ? reason : "(unknown)");

	client_exit(client);

	return offset;
}

void packet_send_disconnect(struct bedrock_client *client, const char *reason)
{
	bedrock_packet packet;

	bedrock_log(LEVEL_INFO, "client: Kicking client %s (%s) - %s", *client->name ? client->name : "(unknown)", client_get_ip(client), reason);

	packet_init(&packet, DISCONNECT);

	packet_pack_header(&packet, DISCONNECT);
	packet_pack_string(&packet, reason);

	client_send_packet(client, &packet);

	io_set(&client->fd, 0, OP_READ);
}
