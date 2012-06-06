#include "server/bedrock.h"
#include "server/config.h"
#include "server/listener.h"
#include "server/client.h"
#include "server/region.h"
#include "io/io.h"
#include "util/timer.h"
#include "packet/packet_keep_alive.h"

#include <time.h>
#include <errno.h>

bool bedrock_running = true;
time_t bedrock_start;
struct timespec bedrock_time = { 0, 0 };
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
	diff = (bedrock_time.tv_sec * 1000000000 + bedrock_time.tv_nsec) - (last_tick.tv_sec * 1000000000 + last_tick.tv_sec);
	/* Get milli seconds */
	diff /= 1000000;

	tick_diff = diff / BEDROCK_TICK_LENGTH;
	if (tick_diff > 0)
	{
		bedrock_node *node;

		last_tick = bedrock_time;

		LIST_FOREACH(&world_list, node)
		{
			struct bedrock_world *world = node->data;
			world->time += tick_diff;
		}

		bedrock_timer_process();
		bedrock_thread_process();
	}
}

void bedrock_log(bedrock_log_level level, const char *msg, ...)
{
	va_list args;
	char buffer[512];

	va_start(args, msg);
	vsnprintf(buffer, sizeof(buffer), msg, args);
	va_end(args);

	if (level != LEVEL_THREAD && level != LEVEL_NBT_DEBUG && level != LEVEL_IO_DEBUG && level != LEVEL_COLUMN && level != LEVEL_PACKET_DEBUG)// && level != LEVEL_BUFFER && level != LEVEL_COLUMN)
		fprintf(stdout, "%s\n", buffer);
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

int main(int argc, char **argv)
{
	struct bedrock_world *world;

	clock_gettime(CLOCK_MONOTONIC, &bedrock_time);
	bedrock_start = time(NULL);
	last_tick = bedrock_time;

	world = world_create(BEDROCK_WORLD_NAME, BEDROCK_WORLD_BASE);
	if (world_load(world) == false)
		exit(1);

	io_init();
	listener_init();

	bedrock_timer_schedule(400, send_keepalive, NULL);
	bedrock_timer_schedule(6000, region_free_queue, NULL);

	while (bedrock_running || client_list.count > 0)
	{
		io_process();
		client_process_exits();
	}

	bedrock_thread_exit_all();

	bedrock_timer_cancel_all_for(send_keepalive);
	bedrock_timer_cancel_all_for(region_free_queue);

	listener_shutdown();
	io_shutdown();
	world_free(world);

	bedrock_assert(bedrock_memory.size == 0, ;);

	return 0;
}
