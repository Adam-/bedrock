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
#define DATA_CHUNK_SIZE 2048

void region_operation_free(struct region_operation *op)
{
	bedrock_buffer_free(op->nbt_out);
	bedrock_free(op);
}

void region_operation_schedule(struct region_operation *op)
{
	bedrock_mutex_lock(&op->region->operations_mutex);
	bedrock_list_add(&op->region->operations, op);
	bedrock_mutex_unlock(&op->region->operations_mutex);
	bedrock_cond_wakeup(&op->region->worker_condition);
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
}

static void region_worker_write(struct region_operation *op)
{
	struct bedrock_column *column = op->column;
	struct bedrock_region *region = op->region;
	int32_t column_x, column_z;
	int offset;
	uint8_t sectors;
	uint32_t structure_start;
	compression_buffer *cb;
	int required_sectors, j;
	uint32_t header_len;
	unsigned char header[5];
	char padding[BEDROCK_REGION_SECTOR_SIZE];

	column_x = column->x % BEDROCK_COLUMNS_PER_REGION;
	if (column_x < 0)
		column_x = BEDROCK_COLUMNS_PER_REGION - abs(column_x);

	column_z = column->z % BEDROCK_COLUMNS_PER_REGION;
	if (column_z < 0)
		column_z = BEDROCK_COLUMNS_PER_REGION - abs(column_z);

	offset = column_x + column_z * BEDROCK_COLUMNS_PER_REGION;
	offset *= sizeof(int32_t);

	bedrock_mutex_lock(&region->fd_mutex);

	if (region->fd.open == false)
	{
		bedrock_mutex_unlock(&region->fd_mutex);
		return;
	}

	// Move to the offset, which contains the offset into the file of where this column starts
	if (lseek(region->fd.fd, offset, SEEK_SET) == -1)
	{
		bedrock_log(LEVEL_WARN, "column: Unable to lseek region file %s to get offset - %s", region->path, strerror(errno));
		bedrock_mutex_unlock(&region->fd_mutex);
		return;
	}

	if (read(region->fd.fd, &structure_start, sizeof(structure_start)) != sizeof(structure_start))
	{
		bedrock_log(LEVEL_WARN, "column: Unable to read column structure offset in file %s to get file offset - %s", region->path, strerror(errno));
		bedrock_mutex_unlock(&region->fd_mutex);
		return;
	}

	convert_endianness((unsigned char *) &structure_start, sizeof(structure_start));

	sectors = structure_start & 0xFF;
	structure_start >>= 8;

	cb = compression_compress_init(DATA_CHUNK_SIZE);
	/* Compress structure */
	compression_compress_deflate_finish(cb, op->nbt_out->data, op->nbt_out->length);

	// Remember, this data will have a 5 byte header!
	required_sectors = (cb->buffer->length + 5) / BEDROCK_REGION_SECTOR_SIZE;
	if ((cb->buffer->length + 5) % BEDROCK_REGION_SECTOR_SIZE)
		++required_sectors;

	bedrock_log(LEVEL_DEBUG, "column: Current offset for %d,%d is %d which takes %d sectors, I need %d sectors", column->x, column->z, structure_start, sectors, required_sectors);

	if (required_sectors <= sectors)
	{
		if (lseek(region->fd.fd, structure_start * BEDROCK_REGION_SECTOR_SIZE, SEEK_SET) == -1)
		{
			bedrock_log(LEVEL_WARN, "column: Unable to lseek region file %s to write structure - %s", region->path, strerror(errno));
			compression_compress_end(cb);
			bedrock_mutex_unlock(&region->fd_mutex);
			return;
		}

		bedrock_log(LEVEL_DEBUG, "column: Number of sectors are sufficient, moved file offset to %d", structure_start);
	}
	else
	{
		off_t pos = lseek(region->fd.fd, 0, SEEK_END);
		uint32_t offset_buffer;

		if (pos == -1)
		{
			bedrock_log(LEVEL_WARN, "column: Unable to lseek region file %s to EOF to write structure - %s", region->path, strerror(errno));
			compression_compress_end(cb);
			bedrock_mutex_unlock(&region->fd_mutex);
			return;
		}
		else if (pos % BEDROCK_REGION_SECTOR_SIZE)
		{
			bedrock_log(LEVEL_WARN, "column: Region file %s does not have correct padding, expecting %d more bytes", region->path, BEDROCK_REGION_SECTOR_SIZE - (pos % BEDROCK_REGION_SECTOR_SIZE));

			j = BEDROCK_REGION_SECTOR_SIZE - (pos % BEDROCK_REGION_SECTOR_SIZE);
			memset(&padding, 0, j);
			if (write(region->fd.fd, padding, j) != j)
			{
				bedrock_log(LEVEL_WARN, "column: Unable to fix padding in region file %s - %s", region->path, strerror(errno));
				compression_compress_end(cb);
				bedrock_mutex_unlock(&region->fd_mutex);
				return;
			}

			pos += j;
		}

		if (lseek(region->fd.fd, offset, SEEK_SET) == -1)
		{
			bedrock_log(LEVEL_WARN, "column: Unable to lseek region file %s to write offset - %s", region->path, strerror(errno));
			compression_compress_end(cb);
			bedrock_mutex_unlock(&region->fd_mutex);
			return;
		}

		bedrock_assert((pos % BEDROCK_REGION_SECTOR_SIZE) == 0, ;);
		bedrock_assert(((pos / BEDROCK_REGION_SECTOR_SIZE) & 0xFF000000) == 0, ;);
		bedrock_assert((required_sectors & ~0xFF) == 0, ;);

		offset_buffer = pos / BEDROCK_REGION_SECTOR_SIZE;
		offset_buffer <<= 8;
		offset_buffer |= required_sectors;

		bedrock_log(LEVEL_DEBUG, "column: Number of sectors are NOT sufficient, moved file offset to %d to write new offset header of position %d with %d sectors", offset, pos / BEDROCK_REGION_SECTOR_SIZE, required_sectors);

		convert_endianness((unsigned char *) &offset_buffer, sizeof(offset_buffer));

		if (write(region->fd.fd, &offset_buffer, sizeof(offset_buffer)) != sizeof(offset_buffer))
		{
			bedrock_log(LEVEL_WARN, "column: Unable to write header for new column in region file %s - %s", region->path, strerror(errno));
			compression_compress_end(cb);
			bedrock_mutex_unlock(&region->fd_mutex);
			return;
		}

		pos = lseek(region->fd.fd, 0, SEEK_END);
		if (pos == -1)
		{
			bedrock_log(LEVEL_WARN, "column: Unable to lseek region file %s to EOF to write structure - %s", region->path, strerror(errno));
			compression_compress_end(cb);
			bedrock_mutex_unlock(&region->fd_mutex);
			return;
		}

		bedrock_assert(pos % BEDROCK_REGION_SECTOR_SIZE == 0, ;);
	}

	// Set up header
	header_len = cb->buffer->length;
	convert_endianness((unsigned char *) &header_len, sizeof(header_len));
	memcpy(&header, &header_len, sizeof(header_len));
	// Compression type
	header[4] = 2;

	if (write(region->fd.fd, header, sizeof(header)) != sizeof(header))
	{
		bedrock_log(LEVEL_WARN, "column: Unable to write column header for region file %s - %s", region->path, strerror(errno));
		compression_compress_end(cb);
		bedrock_mutex_unlock(&region->fd_mutex);
		return;
	}

	if ((size_t) write(region->fd.fd, cb->buffer->data, cb->buffer->length) != cb->buffer->length)
	{
		bedrock_log(LEVEL_WARN, "column: Unable to write column structure for region file %s - %s", region->path, strerror(errno));
		compression_compress_end(cb);
		bedrock_mutex_unlock(&region->fd_mutex);
		return;
	}

	// Pad the end
	j = BEDROCK_REGION_SECTOR_SIZE - ((cb->buffer->length + 5) % BEDROCK_REGION_SECTOR_SIZE);
	bedrock_log(LEVEL_DEBUG, "column: Finished writing compressed NBT structure, file requires an additional %d bytes of padding", j);
	memset(&padding, 0, j);
	if (write(region->fd.fd, padding, j) != j)
	{
		bedrock_log(LEVEL_WARN, "column: Unable to write padding for region file %s - %s", region->path, strerror(errno));
		compression_compress_end(cb);
		bedrock_mutex_unlock(&region->fd_mutex);
		return;
	}

	bedrock_mutex_unlock(&region->fd_mutex);

	compression_compress_end(cb);
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
			case REGION_OP_WRITE:
				region_worker_write(op);
				break;
			default:
				bedrock_log(LEVEL_WARN, "region worker: Unrecognized operation %d", op->operation);
		}

		bedrock_mutex_lock(&region->finished_operations_mutex);
		bedrock_list_add(&region->finished_operations, op);
		bedrock_mutex_unlock(&region->finished_operations_mutex);
		bedrock_pipe_notify(&region->finished_operations_pipe);

		bedrock_mutex_lock(&region->operations_mutex);

		bedrock_assert(op == bedrock_list_del(&region->operations, op), ;);
	}

	bedrock_mutex_unlock(&region->operations_mutex);
}

static void region_operations_notify(struct bedrock_region *region)
{
	bedrock_node *node, *node2;

	bedrock_mutex_lock(&region->finished_operations_mutex);

	LIST_FOREACH_SAFE(&region->finished_operations, node, node2)
	{
		struct region_operation *op = node->data;

		switch (op->operation)
		{
			case REGION_OP_READ:
			{
				op->column->flags &= ~COLUMN_FLAG_READ;

				if (op->column->data == NULL)
				{
					// Column doesn't exist on disk
					column_free(op->column);
				}
				else
				{
					bedrock_node *node;

					bedrock_log(LEVEL_COLUMN, "region: Successfully loaded column at %d, %d from %s", op->column->x, op->column->z, op->region->path);

					LIST_FOREACH(&client_list, node)
					{
						struct bedrock_client *client = node->data;

						if (client->authenticated >= STATE_BURSTING)
						{
							client_update_columns(client);
						}
					}
				}
				break;
			}
			case REGION_OP_WRITE:
				op->column->flags &= ~COLUMN_FLAG_WRITE;

				bedrock_log(LEVEL_COLUMN, "region: Finished save for column %d,%d to %s", op->column->x, op->column->z, op->column->region->path);

				if (op->column->flags & COLUMN_FLAG_EMPTY)
				{
					if (op->column->players.count == 0)
						column_free(op->column);
					else
						op->column->flags &= ~COLUMN_FLAG_EMPTY;
				}
				break;
		}

		bedrock_list_del_node(&region->finished_operations, node);
		bedrock_free(node);
	}

	bedrock_mutex_unlock(&region->finished_operations_mutex);
}

struct bedrock_region *region_create(struct bedrock_world *world, int x, int z)
{
	struct bedrock_region *region = bedrock_malloc(sizeof(struct bedrock_region));
	int fd;

	region->world = world;
	region->x = x;
	region->z = z;
	snprintf(region->path, sizeof(region->path), "%s/region/r.%d.%d.mca", world->path, x, z);

	bedrock_mutex_init(&region->fd_mutex, "region fd mutex");
	fd = open(region->path, O_RDWR);
	if (fd == -1)
		bedrock_log(LEVEL_WARN, "region: Unable to open region file %s - %s", region->path, strerror(errno));
	else
		bedrock_fd_open(&region->fd, fd, FD_FILE, "region file");

	bedrock_mutex_init(&region->operations_mutex, "region operation mutex");
	bedrock_mutex_init(&region->finished_operations_mutex, "region finished operations mutex");
	region->finished_operations.free = (bedrock_free_func) region_operation_free;
	bedrock_pipe_open(&region->finished_operations_pipe, "region operations pipe", (bedrock_pipe_notify_func) region_operations_notify, region);

	bedrock_cond_init(&region->worker_condition, "region condition");

	bedrock_list_add(&world->regions, region);

	region->worker = bedrock_thread_start((bedrock_thread_entry) region_worker, NULL, region);

	return region;
}

void region_free(struct bedrock_region *region)
{
	bedrock_thread_set_exit(region->worker);
	bedrock_cond_wakeup(&region->worker_condition);
	bedrock_thread_join(region->worker);
	bedrock_cond_destroy(&region->worker_condition);

	bedrock_list_del(&region->world->regions, region);

	bedrock_mutex_lock(&region->fd_mutex);
	bedrock_fd_close(&region->fd);
	bedrock_mutex_unlock(&region->fd_mutex);
	bedrock_mutex_destroy(&region->fd_mutex);

	bedrock_mutex_lock(&region->operations_mutex);
	region->operations.free = (bedrock_free_func) region_operation_free;
	bedrock_list_clear(&region->operations);
	bedrock_mutex_unlock(&region->operations_mutex);
	bedrock_mutex_destroy(&region->operations_mutex);

	bedrock_mutex_lock(&region->finished_operations_mutex);
	region->finished_operations.free = (bedrock_free_func) region_operation_free;
	bedrock_list_clear(&region->finished_operations);
	bedrock_mutex_unlock(&region->finished_operations_mutex);
	bedrock_mutex_destroy(&region->finished_operations_mutex);

	bedrock_pipe_close(&region->finished_operations_pipe);

	bedrock_assert(region->columns.count == 0, ;);

	bedrock_free(region);
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
