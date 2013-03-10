#include "server/bedrock.h"
#include "server/region.h"
#include "util/compression.h"
#include "util/file.h"
#include "util/memory.h"
#include "server/world.h"
#include "nbt/nbt.h"
#include "config/hard.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define WORLD_BUFFER_SIZE 4096

bedrock_list world_list = LIST_INIT;

struct world *world_create(const char *name, const char *path)
{
	struct world *world = bedrock_malloc(sizeof(struct world));
	strncpy(world->name, name, sizeof(world->name));
	strncpy(world->path, path, sizeof(world->path));
	bedrock_list_add(&world_list, world);
	return world;
}

bool world_load(struct world *world)
{
	char path[PATH_MAX];
	int fd;
	unsigned char *file_base;
	size_t file_size;
	compression_buffer *cb;
	nbt_tag *tag;

	snprintf(path, sizeof(path), "%s/%s", world->path, BEDROCK_WORLD_LEVEL_FILE);

	fd = open(path, O_RDONLY);
	if (fd == -1)
	{
		bedrock_log(LEVEL_CRIT, "world: Unable to load world information for %s from %s - %s", world->name, path, strerror(errno));
		return false;
	}

	file_base = bedrock_file_read(fd, &file_size);
	if (file_base == NULL)
	{
		bedrock_log(LEVEL_CRIT, "world: Unable to read world information file %s - %s", path, strerror(errno));
		close(fd);
		bedrock_free(file_base);
		return false;
	}

	close(fd);

	cb = compression_decompress(WORLD_BUFFER_SIZE, file_base, file_size);
	bedrock_free(file_base);
	if (cb == NULL)
	{
		bedrock_log(LEVEL_CRIT, "world: Unable to inflate world information file %s", path);
		return false;
	}

	tag = nbt_parse(cb->buffer->data, cb->buffer->length);
	compression_decompress_end(cb);
	if (tag == NULL)
	{
		bedrock_log(LEVEL_CRIT, "world: Unable to NBT parse world information file %s", path);
		return false;
	}

	world->data = tag;
	nbt_copy(world->data, TAG_LONG, &world->time, sizeof(world->time), 2, "Data", "Time");
	bedrock_log(LEVEL_DEBUG, "world: Successfully loaded world information file %s", path);

	return true;
}

struct world_save_info
{
	char name[128];
	char path[PATH_MAX];
	bedrock_buffer *nbt_out;
};

static void world_save_entry(struct bedrock_thread bedrock_attribute_unused *thread, struct world_save_info *wi)
{
	int fd;
	compression_buffer *buffer;

	fd = open(wi->path, O_WRONLY | O_TRUNC | _O_BINARY);
	if (fd == -1)
	{
		bedrock_log(LEVEL_WARN, "world: Unable to open world %s - %s", wi->name, strerror(errno));
		return;
	}

	buffer = compression_compress(WORLD_BUFFER_SIZE, wi->nbt_out->data, wi->nbt_out->length);

	if (write(fd, buffer->buffer->data, buffer->buffer->length) != (int) buffer->buffer->length)
	{
		bedrock_log(LEVEL_WARN, "world: Unable to save world %s - %s", wi->name, strerror(errno));
	}

	compression_compress_end(buffer);

	close(fd);
}

static void world_save_exit(struct world_save_info *wi)
{
	bedrock_log(LEVEL_DEBUG, "world: Finished saving for world %s", wi->name);

	bedrock_buffer_free(wi->nbt_out);
	bedrock_free(wi);
}

void world_save(struct world *world)
{
	struct world_save_info *wi = bedrock_malloc(sizeof(struct world_save_info));

	strncpy(wi->name, world->name, sizeof(wi->name));
	snprintf(wi->path, sizeof(wi->path), "%s/%s", world->path, BEDROCK_WORLD_LEVEL_FILE);
	wi->nbt_out = nbt_write(world->data);

	bedrock_log(LEVEL_DEBUG, "world: Starting save for world %s", world->name);

	bedrock_thread_start((bedrock_thread_entry) world_save_entry, (bedrock_thread_exit) world_save_exit, wi);
}

void world_save_all()
{
	bedrock_node *node;

	LIST_FOREACH(&world_list, node)
	{
		struct world *world = node->data;

		world_save(world);
	}
}

void world_free(struct world *world)
{
	if (bedrock_running)
		world_save(world);

	nbt_free(world->data);

	world->regions.free = (bedrock_free_func) region_free;
	bedrock_list_clear(&world->regions);

	bedrock_list_del(&world_list, world);
	bedrock_free(world);
}

struct world *world_find(const char *name)
{
	bedrock_node *n;

	LIST_FOREACH(&world_list, n)
	{
		struct world *world = n->data;

		if (!strcmp(world->name, name))
			return world;
	}

	return NULL;
}
