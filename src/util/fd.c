#include "util/fd.h"
#include "util/memory.h"
#include "util/list.h"

#include <unistd.h>

bedrock_mutex fdlist_mutex = BEDROCK_MUTEX_INIT("fdlist mutex");
bedrock_list fdlist = LIST_INIT;

void bedrock_fd_open(struct bedrock_fd *f, int fd, bedrock_fd_type type, const char *desc)
{
	bedrock_assert(f != NULL, return);

	f->fd = fd;
	f->type = type;
	if (desc != NULL)
		strncpy(f->desc, desc, sizeof(f->desc));
	f->open = true;

	bedrock_mutex_lock(&fdlist_mutex);
	bedrock_list_add(&fdlist, f);
	bedrock_mutex_unlock(&fdlist_mutex);
}

void bedrock_fd_close(struct bedrock_fd *f)
{
	bedrock_assert(f->open == true, return);

	bedrock_mutex_lock(&fdlist_mutex);
	bedrock_list_del(&fdlist, f);
	bedrock_mutex_unlock(&fdlist_mutex);

	close(f->fd);
	f->open = false;
}

struct bedrock_fd *bedrock_fd_find(int fd)
{
	bedrock_node *node;
	struct bedrock_fd *bfd = NULL;

	bedrock_mutex_lock(&fdlist_mutex);

	LIST_FOREACH(&fdlist, node)
	{
		struct bedrock_fd *f = node->data;

		if (f->fd == fd)
		{
			bfd = f;
			break;
		}
	}

	bedrock_mutex_unlock(&fdlist_mutex);

	return bfd;
}
