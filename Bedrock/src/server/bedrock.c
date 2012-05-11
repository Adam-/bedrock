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

int main(int argc, char **argv)
{
	bedrock_io_init();
	init_listener();

	while (bedrock_running)
	{
		bedrock_io_process();
	}

	bedrock_io_shutdown();

	return 0;
}
