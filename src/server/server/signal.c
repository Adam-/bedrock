#include "server/bedrock.h"
#include "server/client.h"
#include "packet/packet_disconnect.h"
#include "util/io.h"

#include <signal.h>

#ifndef WIN32
static struct event sigbus_signal;
#endif

static struct event sigterm_signal, sigint_signal;

static void signal_handler(evutil_socket_t signum, short bedrock_attribute_unused events, void bedrock_attribute_unused *data)
{
#ifndef WIN32
	bedrock_log(LEVEL_DEBUG, "signal: Got signal %d (%s)", signum, strsignal(signum));
#else
	bedrock_log(LEVEL_DEBUG, "signal: Got signal %d", signum);
#endif

	switch (signum)
	{
#ifndef WIN32
		case SIGBUS:
			break;
#endif
		case SIGTERM:
		case SIGINT:
		{
			bedrock_node *node;

#ifndef WIN32
			bedrock_log(LEVEL_INFO, "signal: Shutting down on %s", strsignal(signum));
#else
			bedrock_log(LEVEL_INFO, "signal: Shutting down on signal %d", signum);
#endif

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
#ifndef WIN32
	io_signal(&sigbus_signal, SIGBUS, signal_handler);
#endif
	io_signal(&sigterm_signal, SIGTERM, signal_handler);
	io_signal(&sigint_signal, SIGINT, signal_handler);
}

void signal_shutdown()
{
#ifndef WIN32
	io_disable(&sigbus_signal);
#endif
	io_disable(&sigterm_signal);
	io_disable(&sigint_signal);
}

