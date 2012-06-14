#include "server/bedrock.h"
#include "server/region.h"
#include "util/memory.h"
#include "compression/compression.h"
#include "server/world.h"
#include "nbt/nbt.h"
#include "server/config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#define WORLD_BUFFER_SIZE 4096
#define WORLD_LEVEL_FILE "level.dat"

struct bedrock_memory_pool world_pool = BEDROCK_MEMORY_POOL_INIT("world memory pool");
bedrock_list world_list =  LIST_INIT;

struct bedrock_world *world_create(const char *name, const char *path)
{
	struct bedrock_world *world = bedrock_malloc_pool(&world_pool, sizeof(struct bedrock_world));
	strncpy(world->name, name, sizeof(world->name));
	strncpy(world->path, path, sizeof(world->path));
	bedrock_list_add(&world_list, world);
	return world;
}

bool world_load(struct bedrock_world *world)
{
	char path[PATH_MAX];
	int fd;
	struct stat file_info;
	unsigned char *file_base;
	compression_buffer *cb;
	nbt_tag *tag;

	snprintf(path, sizeof(path), "%s/%s", world->path, WORLD_LEVEL_FILE);

	fd = open(path, O_RDONLY);
	if (fd == -1)
	{
		bedrock_log(LEVEL_CRIT, "world: Unable to load world information for %s from %s - %s", world->name, path, strerror(errno));
		return false;
	}

	if (fstat(fd, &file_info) != 0)
	{
		bedrock_log(LEVEL_CRIT, "world: Unable to stat world information file %s - %s", path, strerror(errno));
		close(fd);
		return false;
	}

	file_base = mmap(NULL, file_info.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (file_base == MAP_FAILED)
	{
		bedrock_log(LEVEL_CRIT, "world: Unable to map world information file %s - %s", path, strerror(errno));
		close(fd);
		return false;
	}

	close(fd);

	cb = compression_decompress(&world_pool, WORLD_BUFFER_SIZE, file_base, file_info.st_size);
	munmap(file_base, file_info.st_size);
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

void world_free(struct bedrock_world *world)
{
	nbt_free(world->data);

	world->regions.free = (bedrock_free_func) region_free;
	bedrock_list_clear(&world->regions);

	bedrock_list_del(&world_list, world);
	bedrock_free_pool(&world_pool, world);
}

struct bedrock_world *world_find(const char *name)
{
	bedrock_node *n;

	LIST_FOREACH(&world_list, n)
	{
		struct bedrock_world *world = n->data;

		if (!strcmp(world->name, name))
			return world;
	}

	return NULL;
}
