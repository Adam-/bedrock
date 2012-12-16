#include "util/util.h"
#include "getopt.h"

int optind = 0;

int getopt_long(int argc, const char **argv, const char *optstring, const struct option *longopts, int *longindex)
{
	for (; longopts[optind].name != NULL; ++optind)
	{
		int j;

		for (j = 0; j < argc; ++j)
		{
			const char *arg = argv[j];

			if (arg[0] == '-' && arg[1] == '-' && !strcmp(arg + 2, longopts[optind].name))
				;
			else if (arg[0] == '-' && arg[1] == longopts[optind].val)
				;
			else
				continue;

			return longopts[optind].val;
		}
	}

	return -1;
}

