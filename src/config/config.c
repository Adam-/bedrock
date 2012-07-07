#include "server/bedrock.h"
#include "config/hard.h"

#include <errno.h>

char server_desc[BEDROCK_MAX_STRING_LENGTH];
int server_maxusers;
char server_ip[64];
int server_port;

extern FILE *yyin;
extern int yyparse();
extern int config_line_num;

int config_parse(const char *config)
{
	int ret;
	FILE *fd = fopen(config, "r");
	if (fd == NULL)
	{
		bedrock_log(LEVEL_CRIT, "config: Unable to open configuration file %s: %s", config, strerror(errno));
		return -1;
	}

	yyin = fd;
	
	ret = yyparse();

	fclose(fd);

	yylex_destroy();

	return ret;
}

void yyerror(const char *s)
{
	bedrock_log(LEVEL_CRIT, "config: Error parsing line %d: %s", config_line_num, s);
}

