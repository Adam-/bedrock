#include "server/bedrock.h"
#include "server/client.h"
#include "server/command.h"

#include <time.h>

#define MINUTE 60
#define HOUR MINUTE * 60
#define DAY HOUR * 24
#define YEAR DAY * 365

void command_uptime(struct bedrock_command_source *source, int bedrock_attribute_unused argc, const char bedrock_attribute_unused **argv)
{
	struct tm result;
	time_t running = time(NULL) - bedrock_start;
	int years = 0, days = 0, hours = 0, minutes = 0;
	char agobuf[64], timebuf[64];

	if (localtime_r(&bedrock_start, &result) == NULL)
		return;

	while (running >= YEAR)
	{
		++years;
		running -= YEAR;
	}

	while (running >= DAY)
	{
		++days;
		running -= DAY;
	}

	while (running >= HOUR)
	{
		++hours;
		running -= HOUR;
	}

	while (running >= MINUTE)
	{
		++minutes;
		running -= MINUTE;
	}

	agobuf[0] = 0;

	if (years)
	{
		snprintf(timebuf, sizeof(timebuf), "%d year%s ", years, years != 1 ? "s" : "");
		strncat(agobuf, timebuf, sizeof(agobuf));
	}

	if (days)
	{
		snprintf(timebuf, sizeof(timebuf), "%d day%s ", days, days != 1 ? "s" : "");
		strncat(agobuf, timebuf, sizeof(agobuf));
	}

	if (hours)
	{
		snprintf(timebuf, sizeof(timebuf), "%d hour%s ", hours, hours != 1 ? "s" : "");
		strncat(agobuf, timebuf, sizeof(agobuf));
	}

	if (minutes)
	{
		snprintf(timebuf, sizeof(timebuf), "%d minute%s ", minutes, minutes != 1 ? "s" : "");
		strncat(agobuf, timebuf, sizeof(agobuf));
	}

	if (!*agobuf)
	{
		snprintf(timebuf, sizeof(timebuf), "%ld second%s ", running, running != 1 ? "s" : "");
		strncat(agobuf, timebuf, sizeof(timebuf));
	}

	strftime(timebuf, sizeof(timebuf), "%b %d %H:%M:%S %Y %Z", &result);

	command_reply(source, "Started on %s, which is %sago", timebuf, agobuf);
}
