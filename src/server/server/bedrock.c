#include "server/bedrock.h"
#include "server/listener.h"
#include "server/client.h"
#include "server/column.h"
#include "server/signal.h"
#include "packet/packet_keep_alive.h"
#include "config/config.h"
#include "util/crypto.h"
#include "util/io.h"
#include "protocol/console.h"

#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static bool foreground = false;
bool bedrock_running = true;
time_t bedrock_start;
struct timespec bedrock_time = { 0, 0 };
uint32_t entity_id = 0;

uint16_t bedrock_conf_log_level = 0;

static struct timespec last_tick;

static void update_time()
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
			world->creation += tick_diff;
		}
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
	bedrock_node *node;

	va_start(args, msg);
	vsnprintf(buffer, sizeof(buffer) - 1, msg, args);
	va_end(args);

	strncat(buffer, "\n", 1);

	if (bedrock_conf_log_level & level)
	{
		fprintf(stdout, "%s", buffer);

		if (log_fd.open)
			write(log_fd.fd, buffer, strlen(buffer));

		LIST_FOREACH(&console_list, node)
		{
			struct bedrock_console_client *client = node->data;
			console_write(client, buffer);
		}
	}
}

static void bedrock_log_close()
{
	bedrock_fd_close(&log_fd);
}

static void send_keepalive(evutil_socket_t bedrock_attribute_unused fd, short bedrock_attribute_unused events, void bedrock_attribute_unused *data)
{
	bedrock_node *n;
	static uint32_t id = 1;

	LIST_FOREACH(&client_list, n)
	{
		struct bedrock_client *client = n->data;

		if ((client->authenticated & STATE_IN_GAME) && !(client->authenticated & STATE_BURSTING))
			packet_send_keep_alive(client, id);
	}

	while (++id == 0);
}

static void save(evutil_socket_t bedrock_attribute_unused fd, short bedrock_attribute_unused events, void bedrock_attribute_unused *data)
{
	/* Save pending columns */
	column_process_pending();

	/* Save clients */
	client_save_all();

	/* Save worlds (level.dat) */
	world_save_all();
}

static void parse_cli_args(int argc, char **argv)
{
	int c;

	struct option options[] = {
		{"help", no_argument, NULL, 'h'},
		{"foreground", no_argument, NULL, 'f'},
		{"version", no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};

	while ((c = getopt_long(argc, argv, "hfv", options, NULL)) != -1)
	{
		switch (c)
		{
			case 'h':
				fprintf(stdout, "Bedrock %d.%d%s, built on %s at %s\n", BEDROCK_VERSION_MAJOR, BEDROCK_VERSION_MINOR, BEDROCK_VERSION_EXTRA, __DATE__, __TIME__);
				fprintf(stdout, "usage:\n");
				fprintf(stdout, " -h         shows this help\n");
#ifndef WIN32
				fprintf(stdout, " -f         run in foreground\n");
#endif
				fprintf(stdout, " -v         shows version\n");
				fprintf(stdout, "\n");
				exit(0);
				break;
			case 'f':
				foreground = true;
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

static void do_fork()
{
#ifndef WIN32
	int i;

	if (foreground)
		return;

	i = fork();
	if (i <= -1)
	{
		bedrock_log(LEVEL_WARN, "bedrock: Unable to fork into background - %s", strerror(errno));
		return;
	}
	else if (i > 0)
		exit(0);

	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);

	setpgid(0, 0);
#endif
}

int main(int argc, char **argv)
{
	struct bedrock_world *world;
	struct event send_keepalive_timer, save_timer;

	srand(time(NULL));
	bedrock_log_init();

	parse_cli_args(argc, argv);

	if (config_parse("server.config"))
		exit(1);

	bedrock_log(LEVEL_INFO, "Bedrock %d.%d%s starting up", BEDROCK_VERSION_MAJOR, BEDROCK_VERSION_MINOR, BEDROCK_VERSION_EXTRA);
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
	
	do_fork();

	crypto_init();
	io_init();
	signal_init();
	listener_init();
	console_init();
	bedrock_threadengine_start();

	io_timer_schedule(&send_keepalive_timer, 400, EV_PERSIST, send_keepalive, NULL);
	io_timer_schedule(&save_timer, 6000, EV_PERSIST, save, NULL);

	while (bedrock_running || client_list.count > 0)
	{
		io_process();
		update_time();
		client_process_exits();
		console_process_exits();
	}

	io_disable(&send_keepalive_timer);
	io_disable(&save_timer);

	save(-1, 0, NULL);

	world_free(world);

	bedrock_thread_exit_all();
	bedrock_threadengine_stop();

	console_shutdown();
	listener_shutdown();
	signal_shutdown();
	io_shutdown();
	crypto_shutdown();

	bedrock_list_clear(&oper_conf_list);
	bedrock_log_close();

	bedrock_assert(memory_size == 0, ;);
	bedrock_assert(fdlist.count == 0, ;);

	return 0;
}
