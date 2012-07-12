%{
#include "server/world.h"
#include "server/oper.h"
#include "config/config.h"

extern int yylex();
extern void yyerror(const char *s);

static struct bedrock_world *world;
static struct bedrock_oper *oper;

%}

%error-verbose

%union
{
	int ival;
	char *sval;
}

%token <ival> INT
%token <sval> STRING

/* World */
%token WORLD
%token NAME
%token PATH

/* Server */
%token SERVER
%token DESCRIPTION
%token MAXUSERS
%token IP
%token PORT

/* Operator */
%token OPER
%token PASSWORD
%token COMMANDS

%%

conf: | conf conf_items;

conf_items: world_entry | server_entry | operator_entry;

/* World */
world_entry: WORLD
{
	world = bedrock_malloc_pool(&world_pool, sizeof(struct bedrock_world));
}
'{' world_items '}'
{
	bedrock_list_add(&world_list, world);
};

world_items: | world_item world_items;
world_item: world_name | world_path;

world_name: NAME '=' STRING ';'
{
	strncpy(world->name, yylval.sval, sizeof(world->name));
};

world_path: PATH '=' STRING ';'
{
	strncpy(world->path, yylval.sval, sizeof(world->path));
};

/* Server */
server_entry: SERVER '{' server_items '}';

server_items: | server_item server_items;
server_item: server_description | server_maxusers | server_ip | server_port;

server_description: DESCRIPTION '=' STRING ';'
{
	strncpy(server_desc, yylval.sval, sizeof(server_desc));
};

server_maxusers: MAXUSERS '=' INT ';'
{
	server_maxusers = yylval.ival;
};

server_ip: IP '=' STRING ';'
{
	strncpy(server_ip, yylval.sval, sizeof(server_ip));
};

server_port: PORT '=' INT ';'
{
	server_port = yylval.ival;
};

/* Operator */
operator_entry: OPER
{
	oper = bedrock_malloc(sizeof(struct bedrock_oper));
	oper->commands.free = bedrock_free;
}
'{' oper_items '}'
{
	oper_conf_list.free = (bedrock_free_func) oper_free;
	bedrock_list_add(&oper_conf_list, oper);
};

oper_items: | oper_item oper_items;
oper_item: oper_name | oper_password | oper_commands;

oper_name: NAME '=' STRING ';'
{
	strncpy(oper->username, yylval.sval, sizeof(oper->username));
};

oper_password: PASSWORD '=' STRING ';'
{
	strncpy(oper->password, yylval.sval, sizeof(oper->password));
};

oper_commands: COMMANDS '=' STRING ';'
{
	char *start = yylval.sval;
	char *p = strchr(start, ' ');

	while (start != NULL)
	{
		if (p != NULL)
			*p++ = 0;

		bedrock_list_add(&oper->commands, bedrock_strdup(start));

		start = p;
		if (p != NULL)
			p = strchr(start, ' ');
	}
};

%%

