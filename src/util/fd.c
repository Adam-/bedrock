#include "util/fd.h"
#include "util/memory.h"
#include "util/list.h"

#include <unistd.h>

static bedrock_list fdlist;

bedrock_fd *bedrock_fd_open(bedrock_fd *f, int fd, bedrock_fd_type type, const char *desc)
{
	bedrock_assert(f != NULL, return NULL);

	f->fd = fd;
	f->type = type;
	if (desc != NULL)
		strncpy(f->desc, desc, sizeof(f->desc));
	f->ops = 0;
	f->open = true;

	bedrock_list_add(&fdlist, f);

	return f;
}

void bedrock_fd_close(bedrock_fd *f)
{
	bedrock_assert(f->open == true, return);

	bedrock_list_del(&fdlist, f);

	close(f->fd);
	f->open = false;
}

bedrock_fd *bedrock_fd_find(int fd)
{
	bedrock_node *node;

	LIST_FOREACH(&fdlist, node)
	{
		bedrock_fd *f = node->data;

		if (f->fd == fd)
			return f;
	}

	return NULL;
}
