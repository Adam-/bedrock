#include "fd.h"

typedef struct
{
	bedrock_fd read_fd;
	bedrock_fd write_fd;
	void (*on_notify)(void *);
	void *data;
} bedrock_pipe;

extern bedrock_pipe *bedrock_pipe_open(const char *desc, void (*on_notify)(void *), void *data);
extern void bedrock_pipe_close(bedrock_pipe *p);
extern void bedrock_pipe_notify(bedrock_pipe *p);
