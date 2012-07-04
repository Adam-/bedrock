%{
#include "parser.h"

int config_line_num = 1;
%}

%option noyywrap
%option nounput
%option noinput

%%
world		{ return WORLD; }
name		{ return NAME; }
path		{ return PATH; }
server		{ return SERVER; }
description	{ return DESCRIPTION; }
maxusers	{ return MAXUSERS; }
ip		{ return IP; }
port		{ return PORT; }

[ \t]			{ }
\n			{ ++config_line_num; }
[0-9]+			{ yylval.ival = atoi(yytext); return INT; }
\"[^\"\n]*[\"\n]	{ yytext[yyleng - 1] = 0; yylval.sval = yytext + 1; return STRING; }
.			{ return yytext[0]; }

%%

