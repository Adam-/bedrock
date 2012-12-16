
#define no_argument 0

struct option
{
	const char *name;
	int has_arg;
	int *flag;
	int val;
};

extern int optind;

extern int getopt_long(int argc, const char **argv, const char *optstring, const struct option *longopts, int *longindex);