#include "server/bedrock.h"
#include "server/command.h"
#include "io/io.h"
#include "packet/packet_disconnect.h"

void command_shutdown(struct bedrock_client *client, int argc, const char **argv)
{
	bedrock_node *node;
	const char *reason = "Server is shutting down";

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *c = node->data;

		packet_send_disconnect(c, reason);
	}

	bedrock_running = false;
}
