#ifndef BEDROCK_UTIL_LIST_H
#define BEDROCK_UTIL_LIST_H

#include "util.h"

typedef struct _bedrock_node
{
	struct _bedrock_node *next, *prev;
	void *data;
} bedrock_node;

typedef void (*bedrock_free_func)(void *);

typedef struct
{
	bedrock_node *head, *tail;
	size_t count;
	bool (*compare)(const void *data1, const void *data2);
	bedrock_free_func free;
} bedrock_list;

#define LIST_INIT { NULL, NULL, 0, NULL, NULL }
#define LIST_FOREACH(list, var) for (var = (list)->head; var; var = var->next)
#define LIST_FOREACH_SAFE(list, var1, var2) for (var1 = (list)->head, var2 = var1 ? var1->next : NULL; var1; var1 = var2, var2 = var1 ? var1->next : NULL)

extern bool bedrock_list_compare_regular(const void *data1, const void *data2);

extern bedrock_node *bedrock_list_add(bedrock_list *list, void *data);

extern void bedrock_list_add_node(bedrock_list *list, bedrock_node *node, void *data);

extern void bedrock_list_add_node_after(bedrock_list *list, bedrock_node *node, bedrock_node *after, void *data);

extern void bedrock_list_add_node_before(bedrock_list *list, bedrock_node *node, bedrock_node *before, void *data);

extern void *bedrock_list_del(bedrock_list *list, const void *data);

extern void *bedrock_list_del_node(bedrock_list *list, bedrock_node *node);

extern bool bedrock_list_has_data(bedrock_list *list, const void *data);

extern void bedrock_list_clear(bedrock_list *t);

#endif // BEDROCK_UTIL_LIST_H

