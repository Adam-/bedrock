#ifndef BEDROCK_SERVER_REGION_H
#define BEDROCK_SERVER_REGION_H

#include "nbt/tag.h"
#include "util/thread.h"
#include "util/buffer.h"

#include <limits.h>

enum region_op
{
	REGION_OP_READ = 1 << 0,
	REGION_OP_WRITE = 1 << 1
};

/* A pending operation on a region */
struct region_operation
{
	struct region *region;
	unsigned int operation:2;
	struct column *column;
	
	/* Used for REGION_OP_WRITE, contains NBT data to write in binary form. */
	bedrock_buffer *nbt_out;
};

struct region
{
	struct world *world;
	int x;
	int z;
	char path[PATH_MAX];

	/* Must be held for this fd */
	bedrock_mutex fd_mutex;
	/* FD for this region */
	struct bedrock_fd fd;

	/* Must be held for accessing operations! */
	bedrock_mutex operations_mutex;
	/* Pending operations on this region, list of region_operation structures */
	bedrock_list operations;
	/* Must be held for accessing finished_operations! */
	bedrock_mutex finished_operations_mutex;
	/* A list of finished operations */
	bedrock_list finished_operations;
	/* Used to notify us when an operation completes */
	bedrock_pipe finished_operations_pipe;

	/* Worker thread for this region */
	struct bedrock_thread *worker;
	/* condition for worker */
	bedrock_cond worker_condition;

	/* Columns in this region */
	bedrock_list columns;
};

extern void region_operation_free(struct region_operation *op);
extern void region_operation_schedule(struct region_operation *op);
extern struct region *region_create(struct world *world, int x, int z);
extern void region_free(struct region *region);
/* Finds the region which contains the point x and z */
extern struct region *find_region_which_contains(struct world *world, double x, double z);

#endif // BEDROCK_SERVER_REGION_H
