#include "server/bedrock.h"
#include "io/io.h"

void bedrock_log(bedrock_log_level level, const char *msg, ...)
{
	va_list args;
	char buffer[512];

	va_start(args, msg);
	vsnprintf(buffer, sizeof(buffer), msg, args);
	va_end(args);

	if (level != LEVE_NBT_DEBUG)
		fprintf(stdout, "%s\n", buffer);
}

bool bedrock_running = true;

#include "server/region.h"

int main(int argc, char **argv)
{
	bedrock_region *region = bedrock_region_create("/home/adam/cNBT/r.0.0.mca", 0, 0);
	bedrock_node *n;

	bedrock_region_load(region);

	printf("List size %d\n", region->columns.count);

	LIST_FOREACH(&region->columns, n)
	{
		nbt_tag *tag = n->data;
		nbt_ascii_dump(tag);
	}

	bedrock_region_free(region);
	printf("Done!\n");

	/*bedrock_io_init();
	init_listener();

	while (bedrock_running)
	{
		bedrock_io_process();
	}

	bedrock_io_shutdown();*/

	return 0;
}
