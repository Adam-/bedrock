#include "server/bedrock.h"
#include "server/io.h"
#include "server/client.h"
#include "packet/packet_disconnect.h"

#include <signal.h>

static struct event sigbus_signal, sigterm_signal, sigint_signal;

static void signal_handler(evutil_socket_t signum, short bedrock_attribute_unused events, void bedrock_attribute_unused *data)
{
	bedrock_log(LEVEL_DEBUG, "signal: Got signal %d (%s)", signum, strsignal(signum));

	switch (signum)
	{
		case SIGBUS:
			break;
		case SIGTERM:
		case SIGINT:
		{
			bedrock_node *node;

			bedrock_log(LEVEL_INFO, "signal: Shutting down on %s", strsignal(signum));

			LIST_FOREACH(&client_list, node)
			{
				struct bedrock_client *c = node->data;

				packet_send_disconnect(c, "Server is shutting down");
			}

			bedrock_running = false;
		}
	}
}

void signal_init()
{
	io_signal(&sigbus_signal, SIGBUS, signal_handler);
	io_signal(&sigterm_signal, SIGTERM, signal_handler);
	io_signal(&sigint_signal, SIGINT, signal_handler);
}

void signal_shutdown()
{
	io_disable(&sigbus_signal);
	io_disable(&sigterm_signal);
	io_disable(&sigint_signal);
}

