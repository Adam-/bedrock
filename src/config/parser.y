%{
#include "lexer.h"
#include "server/world.h"
#include "config/config.h"

extern void yyerror(const char *s);

static struct bedrock_world *world;

%}

%error-verbose

%union
{
	int ival;
	char *sval;
}

%token <ival> INT
%token <sval> STRING

%token WORLD
%token NAME
%token PATH
%token SERVER
%token DESCRIPTION
%token MAXUSERS
%token IP
%token PORT

%%

conf: | conf conf_items;

conf_items: world_entry | server_entry;

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

%%

