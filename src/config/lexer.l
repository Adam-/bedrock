%{
#include "parser.h"

int config_line_num = 1;
%}

%option noyywrap
%option nounput
%option noinput

%%

 /* World */
world		{ return WORLD; }
name		{ return NAME; }
path		{ return PATH; }

 /* Server */
server		{ return SERVER; }
description	{ return DESCRIPTION; }
maxusers	{ return MAXUSERS; }
ip		{ return IP; }
port		{ return PORT; }
log_level       { return LOG_LEVEL; }
CRIT            { return CRIT; }
WARN            { return WARN; }
INFO            { return INFO; }
DEBUG           { return DEBUG; }
COLUMN          { return COLUMN; }
NBT_DEBUG       { return NBT_DEBUG; }
THREAD          { return THREAD; }
BUFFER          { return BUFFER; }
IO_DEBUG        { return IO_DEBUG; }
PACKET_DEBUG    { return PACKET_DEBUG; }

 /* Operator */
oper            { return OPER; }
 /* name            { return NAME; } */
password        { return PASSWORD; }
commands        { return COMMANDS; }

[ \t]			{ }
\n			{ ++config_line_num; }
[0-9]+			{ yylval.ival = atoi(yytext); return INT; }
\"[^\"\n]*[\"\n]	{ yytext[yyleng - 1] = 0; yylval.sval = yytext + 1; return STRING; }
#.*                     { }
.			{ return yytext[0]; }

%%

