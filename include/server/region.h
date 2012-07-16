#include "nbt/tag.h"
#include "util/thread.h"

#include <limits.h>

enum region_op
{
	REGION_OP_READ = 1 << 0,
	REGION_OP_WRITE = 1 << 1
};

/* A pending operation on a region */
struct region_operation
{
	struct bedrock_region *region;
	uint8_t operation:2;
	struct bedrock_column *column;
};

struct bedrock_region
{
	struct bedrock_world *world;
	int x;
	int z;
	char path[PATH_MAX];

	/* FD for this region */
	struct bedrock_fd fd;
	/* Must be held for this fd */
	bedrock_mutex fd_mutex;
	/* Must be held for accessing operations! */
	bedrock_mutex operations_mutex;
	/* Pending operations on this region, list of region_operation structures */
	bedrock_list operations;
	/* Worker thread for this region */
	struct bedrock_thread *worker;
	/* condition for worker */
	bedrock_cond worker_condition;

	/* Must be held when reading or writing to columns! */
	bedrock_mutex column_mutex;//XXX this needed?
	/* Columns in this region */
	bedrock_list columns;
};

extern struct bedrock_memory_pool region_pool;

extern struct bedrock_region *region_create(struct bedrock_world *world, int x, int z);
extern void region_free(struct bedrock_region *region);
extern void region_schedule_operation(struct bedrock_region *region, struct bedrock_column *column, enum region_op op);
/* Finds the region which contains the point x and z */
extern struct bedrock_region *find_region_which_contains(struct bedrock_world *world, double x, double z);
