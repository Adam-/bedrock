#include "server/bedrock.h"
#include "util/endian.h"
#include "nbt/nbt.h"
#include "compression/compression.h"
#include "util/memory.h"
#include "server/world.h"
#include "server/column.h"
#include "util/timer.h"
#include "server/client.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

#define REGION_BUFFER_SIZE 65536

static bedrock_pipe region_worker_read_pipe;

static void region_worker_read_exit(void __attribute__((__unused__)) *unused)
{
	bedrock_node *node;

	LIST_FOREACH(&client_list, node)
	{
		struct bedrock_client *client = node->data;

		if (client->authenticated >= STATE_BURSTING)
		{
			client_update_columns(client);
		}
	}
}

void region_init()
{
	bedrock_pipe_open(&region_worker_read_pipe, "region worker read pipe", region_worker_read_exit, NULL);
}

void region_shutdown()
{
	bedrock_pipe_close(&region_worker_read_pipe);
}

static void region_worker_read(struct region_operation *op)
{
	int32_t column_x, column_z;
	int offset;
	uint32_t read_offset;
	uint32_t compressed_len;
	uint8_t compression_type;
	unsigned char *buffer;
	compression_buffer *cb;
	nbt_tag *tag;

	column_x = op->column->x % BEDROCK_COLUMNS_PER_REGION;
	if (column_x < 0)
		column_x = BEDROCK_COLUMNS_PER_REGION - abs(column_x);

	column_z = op->column->z % BEDROCK_COLUMNS_PER_REGION;
	if (column_z < 0)
		column_z = BEDROCK_COLUMNS_PER_REGION - abs(column_z);

	offset = column_x + column_z * BEDROCK_COLUMNS_PER_REGION;
	offset *= sizeof(int32_t);

	bedrock_mutex_lock(&op->region->fd_mutex);

	if (op->region->fd.open == false)
	{
		bedrock_mutex_unlock(&op->region->fd_mutex);
		return;
	}

	if (lseek(op->region->fd.fd, offset, SEEK_SET) == -1)
	{
		bedrock_mutex_unlock(&op->region->fd_mutex);
		bedrock_log(LEVEL_WARN, "Unable to lseek %s to read structure offset - %s", op->region->path, strerror(errno));
		return;
	}

	if (read(op->region->fd.fd, &read_offset, sizeof(read_offset)) != sizeof(read_offset))
	{
		bedrock_mutex_unlock(&op->region->fd_mutex);
		bedrock_log(LEVEL_WARN, "Unable to read %s to get compressed data offset - %s", op->region->path, strerror(errno));
		return;
	}
	convert_endianness((unsigned char *) &read_offset, sizeof(read_offset));

	if (read_offset == 0)
	{
		bedrock_mutex_unlock(&op->region->fd_mutex);
		return;
	}

	read_offset >>= 8;

	if (lseek(op->region->fd.fd, read_offset * BEDROCK_REGION_SECTOR_SIZE, SEEK_SET) == -1)
	{
		bedrock_mutex_unlock(&op->region->fd_mutex);
		bedrock_log(LEVEL_WARN, "Unable to lseek %s to read compressed data length - %s", op->region->path, strerror(errno));
		return;
	}

	if (read(op->region->fd.fd, &compressed_len, sizeof(compressed_len)) != sizeof(compressed_len))
	{
		bedrock_mutex_unlock(&op->region->fd_mutex);
		bedrock_log(LEVEL_WARN, "Unable to read %s to get compressed data length - %s", op->region->path, strerror(errno));
		return;
	}
	convert_endianness((unsigned char *) &compressed_len, sizeof(compressed_len));

	if (read(op->region->fd.fd, &compression_type, sizeof(compression_type)) != sizeof(compression_type))
	{
		bedrock_mutex_unlock(&op->region->fd_mutex);
		bedrock_log(LEVEL_WARN, "Unable to read %s to get compression type - %s", op->region->path, strerror(errno));
		return;
	}

	buffer = bedrock_malloc(compressed_len);
	if (read(op->region->fd.fd, buffer, compressed_len) != compressed_len)
	{
		bedrock_mutex_unlock(&op->region->fd_mutex);
		bedrock_free(buffer);
		bedrock_log(LEVEL_WARN, "Unable to read %s to get compressed data structure - %s", op->region->path, strerror(errno));
		return;
	}

	bedrock_mutex_unlock(&op->region->fd_mutex);

	cb = compression_decompress(REGION_BUFFER_SIZE, buffer, compressed_len);

	bedrock_free(buffer);

	tag = nbt_parse(cb->buffer->data, cb->buffer->length);

	compression_decompress_end(cb);

	column_load(op->column, tag);

	op->column->flags &= ~COLUMN_FLAG_READ;

	bedrock_log(LEVEL_COLUMN, "region: Successfully loaded column at %d, %d from %s", op->column->x, op->column->z, op->region->path);

	bedrock_pipe_notify(&region_worker_read_pipe);
}

static void region_worker(struct bedrock_thread *thread, struct bedrock_region *region)
{
	bedrock_mutex_lock(&region->operations_mutex);

	while ((region->operations.count > 0 || bedrock_cond_wait(&region->worker_condition, &region->operations_mutex)) && bedrock_thread_want_exit(thread) == false)
	{
		struct region_operation *op;

		if (region->operations.count == 0)
			continue;

		op = region->operations.head->data;

		bedrock_mutex_unlock(&region->operations_mutex);

		switch (op->operation)
		{
			case REGION_OP_READ:
				region_worker_read(op);
				break;
			default:
				bedrock_log(LEVEL_WARN, "region worker: Unrecognized operation %d", op->operation);
		}

		bedrock_mutex_lock(&region->operations_mutex);

		bedrock_assert(op == bedrock_list_del(&region->operations, op), ;);
		bedrock_free(op);
	}

	bedrock_mutex_unlock(&region->operations_mutex);
}

struct bedrock_region *region_create(struct bedrock_world *world, int x, int z)
{
	struct bedrock_region *region = bedrock_malloc(sizeof(struct bedrock_region));
	int fd;

	region->world = world;
	region->x = x;
	region->z = z;
	snprintf(region->path, sizeof(region->path), "%s/region/r.%d.%d.mca", world->path, x, z);

	fd = open(region->path, O_RDONLY);// O_RDWR
	if (fd == -1)
		bedrock_log(LEVEL_WARN, "region: Unable to open region file %s - %s", region->path, strerror(errno));
	else
		bedrock_fd_open(&region->fd, fd, FD_FILE, "region file");
	bedrock_mutex_init(&region->fd_mutex, "region fd mutex");

	bedrock_mutex_init(&region->operations_mutex, "region operation mutex");

	region->columns.free = (bedrock_free_func) column_free;

	bedrock_list_add(&world->regions, region);

	bedrock_cond_init(&region->worker_condition, "region condition");
	region->worker = bedrock_thread_start((bedrock_thread_entry) region_worker, NULL, region);

	return region;
}

void region_free(struct bedrock_region *region)
{
	bedrock_cond_wakeup(&region->worker_condition);
	bedrock_thread_join(region->worker);
	bedrock_cond_destroy(&region->worker_condition);

	bedrock_list_del(&region->world->regions, region);

	bedrock_mutex_lock(&region->fd_mutex);
	bedrock_fd_close(&region->fd);
	bedrock_mutex_unlock(&region->fd_mutex);
	bedrock_mutex_destroy(&region->fd_mutex);

	bedrock_mutex_lock(&region->operations_mutex);
	region->operations.free = bedrock_free;
	bedrock_list_clear(&region->operations);
	bedrock_mutex_unlock(&region->operations_mutex);
	bedrock_mutex_destroy(&region->operations_mutex);

	bedrock_list_clear(&region->columns);

	bedrock_free(region);
}

void region_schedule_operation(struct bedrock_region *region, struct bedrock_column *column, enum region_op op)
{
	struct region_operation *operation = bedrock_malloc(sizeof(struct region_operation));
	operation->region = region;
	operation->operation = op;
	operation->column = column;

	bedrock_mutex_lock(&region->operations_mutex);
	bedrock_list_add(&region->operations, operation);
	bedrock_mutex_unlock(&region->operations_mutex);
	bedrock_cond_wakeup(&region->worker_condition);
}

struct bedrock_region *find_region_which_contains(struct bedrock_world *world, double x, double z)
{
	double column_x, column_z;
	double region_x, region_z;
	bedrock_node *n;
	struct bedrock_region *region;

	column_x = x / BEDROCK_BLOCKS_PER_CHUNK, column_z = z / BEDROCK_BLOCKS_PER_CHUNK;
	column_x = floor(column_x);
	column_z = floor(column_z);

	region_x = column_x / BEDROCK_COLUMNS_PER_REGION, region_z = column_z / BEDROCK_COLUMNS_PER_REGION;
	region_x = floor(region_x);
	region_z = floor(region_z);

	LIST_FOREACH(&world->regions, n)
	{
		region = n->data;

		if (region->x == region_x && region->z == region_z)
			return region;
	}

	region = region_create(world, region_x, region_z);

	return region;
}
