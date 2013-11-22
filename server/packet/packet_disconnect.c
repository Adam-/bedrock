#include "server/client.h"
#include "server/packet.h"
#include "server/bedrock.h"
#include "util/io.h"
#include <jansson.h>

void packet_send_disconnect(struct client *client, const char *reason)
{
	bedrock_packet packet;
	json_t *j;
	char *c;

	bedrock_log(LEVEL_INFO, "client: Kicking client %s (%s) - %s", *client->name ? client->name : "(unknown)", client_get_ip(client), reason);

	j = json_pack("{"
				"s: s"
			"}",
				"text", reason);
	c = json_dumps(j, 0);
	bedrock_assert(c != NULL, return;);

	packet_init(&packet, SERVER_DISCONNECT);
	packet_pack_string(&packet, c);
	client_send_packet(client, &packet);

	free(c);
	json_decref(j);

	io_disable(&client->fd.event_read);
}
