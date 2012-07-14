#include "fd.h"

typedef struct
{
	struct bedrock_fd read_fd;
	struct bedrock_fd write_fd;
	void (*on_notify)(void *);
	void *data;
} bedrock_pipe;

extern void bedrock_pipe_open(bedrock_pipe *pipe, const char *desc, void (*on_notify)(void *), void *data);
extern void bedrock_pipe_close(bedrock_pipe *p);
extern void bedrock_pipe_notify(bedrock_pipe *p);
