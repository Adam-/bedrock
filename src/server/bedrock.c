#include "server/bedrock.h"
#include "server/listener.h"
#include "server/client.h"
#include "server/column.h"
#include "io/io.h"
#include "util/timer.h"
#include "packet/packet_keep_alive.h"
#include "config/config.h"

#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

bool bedrock_running = true;
time_t bedrock_start;
struct timespec bedrock_time = { 0, 0 };
uint32_t entity_id = 0;

uint16_t bedrock_conf_log_level = 0;

static struct timespec last_tick;

void bedrock_update_time()
{
	uint64_t diff;
	uint16_t tick_diff;

	if (clock_gettime(CLOCK_MONOTONIC, &bedrock_time) == -1)
	{
		bedrock_log(LEVEL_WARN, "bedrock: Unable to update clock - %s", strerror(errno));
		return;
	}

	/* This is in nano seconds, which is 10^-9 */
	diff = (bedrock_time.tv_sec * 1000000000 + bedrock_time.tv_nsec) - (last_tick.tv_sec * 1000000000 + last_tick.tv_nsec);
	/* Get milli seconds */
	diff /= 1000000;

	tick_diff = diff / BEDROCK_TICK_LENGTH;
	if (tick_diff > 0)
	{
		bedrock_node *node;

		last_tick.tv_nsec += (tick_diff * BEDROCK_TICK_LENGTH) * 1000000;

		last_tick.tv_sec += last_tick.tv_nsec / 1000000000;
		last_tick.tv_nsec %= 1000000000;

		LIST_FOREACH(&world_list, node)
		{
			struct bedrock_world *world = node->data;
			world->time += tick_diff;
		}

		bedrock_timer_process();
	}
}

static struct bedrock_fd log_fd;

static void bedrock_log_init()
{
	int fd = open("server.log", O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
	if (fd == -1)
	{
		bedrock_log(LEVEL_CRIT, "Unable to open log file: %s", strerror(errno));
		return;
	}

	bedrock_fd_open(&log_fd, fd, FD_FILE, "server log");
}

void bedrock_log(bedrock_log_level level, const char *msg, ...)
{
	va_list args;
	char buffer[512];

	va_start(args, msg);
	vsnprintf(buffer, sizeof(buffer), msg, args);
	va_end(args);

	if (level != LEVEL_THREAD && level != LEVEL_NBT_DEBUG && level != LEVEL_IO_DEBUG && /*level != LEVEL_COLUMN && */level != LEVEL_PACKET_DEBUG)// && level != LEVEL_BUFFER && level != LEVEL_COLUMN)
	//if (bedrock_conf_log_level & level)
	{
		fprintf(stdout, "%s\n", buffer);
		if (log_fd.open)
		{
			write(log_fd.fd, buffer, strlen(buffer));
			write(log_fd.fd, "\n", 1);
		}
	}
}

static void bedrock_log_close()
{
	bedrock_fd_close(&log_fd);
}

static void send_keepalive(void __attribute__((__unused__)) *notused)
{
	bedrock_node *n;
	static uint32_t id = 1;

	LIST_FOREACH(&client_list, n)
	{
		struct bedrock_client *client = n->data;

		packet_send_keep_alive(client, id);
	}

	while (++id == 0);

	bedrock_timer_schedule(400, send_keepalive, NULL);
}

static void parse_cli_args(int argc, char **argv)
{
	int c;

	struct option options[] = {
		{"help", no_argument, NULL, 'h'},
		{"version", no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "hv", options, NULL)) != -1)
	{
		switch (c)
		{
			case 'h':
				fprintf(stdout, "Bedrock %d.%d%s, built on %s at %s\n", BEDROCK_VERSION_MAJOR, BEDROCK_VERSION_MINOR, BEDROCK_VERSION_EXTRA, __DATE__, __TIME__);
				fprintf(stdout, "usage:\n");
				fprintf(stdout, " -h         shows this help\n");
				fprintf(stdout, " -v         shows version\n");
				fprintf(stdout, "\n");
				exit(0);
				break;
			case 'v':
				fprintf(stdout, "Bedrock %d.%d%s, built on %s at %s\n", BEDROCK_VERSION_MAJOR, BEDROCK_VERSION_MINOR, BEDROCK_VERSION_EXTRA, __DATE__, __TIME__);
				exit(0);
				break;
			case '?':
				exit(1);
		}
	}
}

#include <signal.h> // XXX
int main(int argc, char **argv)
{
	signal(SIGPIPE, SIG_IGN); // XXX
	struct bedrock_world *world;

	bedrock_log_init();

	parse_cli_args(argc, argv);

	bedrock_log(LEVEL_INFO, "Bedrock %d.%d%s starting up", BEDROCK_VERSION_MAJOR, BEDROCK_VERSION_MINOR, BEDROCK_VERSION_EXTRA);

	if (config_parse("server.config"))
		exit(1);

	bedrock_log(LEVEL_INFO, "Listening on %s:%d with %d max players - %s", server_ip, server_port, server_maxusers, server_desc);

	if (world_list.count == 0)
		exit(1);
	
	world = world_list.head->data;

	bedrock_log(LEVEL_INFO, "Using world \"%s\" at %s", world->name, world->path);

	bedrock_start = time(NULL);
	clock_gettime(CLOCK_MONOTONIC, &bedrock_time);
	last_tick = bedrock_time;

	if (world_load(world) == false)
		exit(1);

	io_init();
	listener_init();
	bedrock_threadengine_start();

	bedrock_timer_schedule(400, send_keepalive, NULL);
	bedrock_timer_schedule(6000, column_process_pending, NULL);

	while (bedrock_running || client_list.count > 0)
	{
		io_process();
		client_process_exits();
	}

	column_process_pending(NULL);

	world_free(world);

	bedrock_thread_exit_all();
	bedrock_threadengine_stop();

	bedrock_timer_cancel_all_for(NULL);

	listener_shutdown();
	io_shutdown();
	bedrock_list_clear(&oper_conf_list);
	bedrock_log_close();

	bedrock_assert(memory_size == 0, ;);
	bedrock_assert(fdlist.count == 0, ;);

	return 0;
}
