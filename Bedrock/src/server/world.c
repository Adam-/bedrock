#include "server/world.h"
#include "server/region.h"
#include "util/memory.h"

bedrock_list world_list;

bedrock_world *bedrock_world_create(const char *name, const char *path)
{
	bedrock_world *world = bedrock_malloc(sizeof(bedrock_world));
	world->regions.free = bedrock_region_free;
	bedrock_list_add(&world_list, world);
	return world;
}

void bedrock_world_free(bedrock_world *world)
{
	bedrock_list_clear(&world->regions);
	bedrock_list_del(&world_list, world);
	bedrock_free(world);
}
