#include "server/bedrock.h"
#include "server/command.h"
#include "util/fd.h"

#include <sys/time.h>
#include <sys/resource.h>

void command_fdlist(struct bedrock_client *client, int argc, const char **argv)
{
	bedrock_node *node;
	int engines = 0, files = 0, sockets = 0, pipes = 0, total = 0;
	struct rlimit rlim;

	LIST_FOREACH(&fdlist, node)
	{
		bedrock_fd *fd = node->data;
		char *type = "Unknown";

		switch (fd->type)
		{
			case FD_ENGINE:
				++engines;
				break;
			case FD_FILE:
				++files;
				break;
			case FD_SOCKET:
				++sockets;
				break;
			case FD_PIPE:
				++pipes;
				break;
			default:
				break;
		}

		++total;

		command_reply(client, "#%d - %s - %s", fd->fd, type, fd->desc);
	}

	command_reply(client, "Total: %d open FDs, %d engine, %d files, %d pipes, and %d sockets", total, engines, files, pipes, sockets);

	if (!getrlimit(RLIMIT_NOFILE, &rlim))
		command_reply(client, "Soft limit: %d, Hard limit: %d", rlim.rlim_cur, rlim.rlim_max);
}
