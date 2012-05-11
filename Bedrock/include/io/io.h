#include "util/fd.h"

typedef enum
{
	OP_READ = 1 << 0,
	OP_WRITE = 1 << 1
} bedrock_io_ops;

extern void bedrock_io_init();
extern void bedrock_io_shutdown();
extern void bedrock_io_set(bedrock_fd *fd, bedrock_io_ops add, bedrock_io_ops remove);
extern void bedrock_io_process();
