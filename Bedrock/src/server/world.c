#include "server/bedrock.h"
#include "server/region.h"
#include "util/memory.h"
#include "compression/compression.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#define BEDROCK_WORLD_LEVEL_FILE "level.dat"

bedrock_list world_list;

bedrock_world *world_create(const char *name, const char *path)
{
	bedrock_world *world = bedrock_malloc(sizeof(bedrock_world));
	strncpy(world->name, name, sizeof(world->name));
	strncpy(world->path, path, sizeof(world->path));
	world->regions.free = region_free;
	bedrock_list_add(&world_list, world);
	return world;
}

bool world_load(bedrock_world *world)
{
	char path[PATH_MAX];
	int fd;
	struct stat file_info;
	unsigned char *file_base;
	compression_buffer *cb;
	nbt_tag *tag;

	snprintf(path, sizeof(path), "%s/%s", world->path, BEDROCK_WORLD_LEVEL_FILE);

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

	cb = compression_decompress(file_base, file_info.st_size);
	munmap(file_base, file_info.st_size);
	if (cb == NULL)
	{
		bedrock_log(LEVEL_CRIT, "world: Unable to inflate world information file %s", path);
		return false;
	}

	tag = nbt_parse(cb->data, cb->length);
	compression_free_buffer(cb);
	if (tag == NULL)
	{
		bedrock_log(LEVEL_CRIT, "world: Unable to NBT parse world information file %s", path);
		return false;
	}

	world->data = tag;
	bedrock_log(LEVEL_DEBUG, "world: Successfully loaded world information file %s", path);

	return true;
}

void world_free(bedrock_world *world)
{
	if (world->data != NULL)
		nbt_free(world->data);
	bedrock_list_clear(&world->regions);

	bedrock_list_del(&world_list, world);
	bedrock_free(world);
}

bedrock_world *world_find(const char *name)
{
	bedrock_node *n;

	LIST_FOREACH(&world_list, n)
	{
		bedrock_world *world = n->data;

		if (!strcmp(world->name, name))
			return world;
	}

	return NULL;
}