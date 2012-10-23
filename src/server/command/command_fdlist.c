#include "server/bedrock.h"
#include "server/command.h"
#include "util/fd.h"

#include <sys/time.h>
#ifndef WIN32
#include <sys/resource.h>
#endif

void command_fdlist(struct bedrock_command_source *source, int bedrock_attribute_unused argc, const char bedrock_attribute_unused **argv)
{
	bedrock_node *node;
	int files = 0, sockets = 0, pipes = 0, total = 0;
#ifndef WIN32
	struct rlimit rlim;
#endif

	bedrock_mutex_lock(&fdlist_mutex);
	LIST_FOREACH(&fdlist, node)
	{
		struct bedrock_fd *fd = node->data;
		char *type = "Unknown";

		switch (fd->type)
		{
			case FD_FILE:
				type = "File";
				++files;
				break;
			case FD_SOCKET:
				type = "Socket";
				++sockets;
				break;
			case FD_PIPE:
				type = "Pipe";
				++pipes;
				break;
			default:
				break;
		}

		++total;

		command_reply(source, "#%d - %s - %s", fd->fd, type, fd->desc);
	}
	bedrock_mutex_unlock(&fdlist_mutex);

	command_reply(source, "Total: %d open FDs, %d files, %d pipes, and %d sockets", total, files, pipes, sockets);

#ifndef WIN32
	if (!getrlimit(RLIMIT_NOFILE, &rlim))
		command_reply(source, "Soft limit: %d, Hard limit: %d", rlim.rlim_cur, rlim.rlim_max);
#endif
}
