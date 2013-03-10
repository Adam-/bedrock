#include "server/oper.h"
#include "util/memory.h"

bedrock_list oper_conf_list = LIST_INIT;

void oper_free(struct oper *oper)
{
	bedrock_list_clear(&oper->commands);
	bedrock_free(oper);
}

struct oper *oper_find(const char *name)
{
	bedrock_node *node;

	LIST_FOREACH(&oper_conf_list, node)
	{
		struct oper *oper = node->data;

		if (!strcmp(name, oper->username))
			return oper;
	}

	return NULL;
}

bool oper_has_command(struct oper *oper, const char *command)
{
	bedrock_node *node;

	if (oper == NULL)
		return false;
	
	LIST_FOREACH(&oper->commands, node)
	{
		const char *oper_cmd = node->data;

		if (!strcasecmp(command, oper_cmd))
			return true;
	}

	return false;
}

