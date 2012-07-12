#include "util/list.h"

struct bedrock_oper
{
	char username[32];
	char password[32];
	bedrock_list commands;
};

extern bedrock_list oper_conf_list;

extern void oper_free(struct bedrock_oper *oper);
extern struct bedrock_oper *oper_find(const char *name);
extern bool oper_has_command(struct bedrock_oper *oper, const char *command);

