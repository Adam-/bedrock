#include "server/bedrock.h"
#include "io/io.h"

void bedrock_log(bedrock_log_level level, const char *msg, ...)
{
	va_list args;
	char buffer[512];

	va_start(args, msg);
	vsnprintf(buffer, sizeof(buffer), msg, args);
	va_end(args);

	fprintf(stdout, "%s\n", buffer);
}

bool bedrock_running = true;

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include "nbt/nbt.h"
#include <errno.h>
#include <fcntl.h>

static void dump_tag(nbt_tag *t)
{
	if (t->name)
		printf("Tag name %s\n", t->name);
	else
		printf("NO tag name\n");
}

int main(int argc, char **argv)
{
	struct stat sb;
	int fd = open("/home/adam/cNBT/level.dat", O_RDONLY);
	assert(fd >= 0);
	assert(fstat(fd, &sb) == 0);
	printf("Size %d\n", sb.st_size);
	char *p = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
	printf("%d %d %d %s\n", errno, EBADF, EINVAL, strerror(errno));
	assert(p != MAP_FAILED);
	close(fd);

	nbt_tag *t = nbt_parse(p, sb.st_size);
	assert(t);
	dump_tag(t);
	printf("DONE\n");

	/*bedrock_io_init();
	init_listener();

	while (bedrock_running)
	{
		bedrock_io_process();
	}

	bedrock_io_shutdown();*/

	return 0;
}
