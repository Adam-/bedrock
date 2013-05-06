#ifndef BEDROCK_SERVER_OPER_H
#define BEDROCK_SERVER_OPER_H

#include "util/list.h"

struct oper
{
	char username[32];
	char password[32];
	bedrock_list commands;
};

extern bedrock_list oper_conf_list;

extern void oper_free(struct oper *oper);
extern struct oper *oper_find(const char *name);
extern bool oper_has_command(struct oper *oper, const char *command);

#endif // BEDROCK_SERVER_OPER_H
