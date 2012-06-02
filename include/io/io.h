#include "util/fd.h"

typedef enum
{
	OP_READ = 1 << 0,
	OP_WRITE = 1 << 1
} bedrock_io_ops;

extern void io_init();
extern void io_shutdown();
extern void io_set(bedrock_fd *fd, bedrock_io_ops add, bedrock_io_ops remove);
extern bool io_has(bedrock_fd *fd, bedrock_io_ops flag);
extern void io_process();
