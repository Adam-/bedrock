#include "util/list.h"
#include "util/memory.h"

/** The default way to compare items in list
 * @param data1 The first item
 * @param data2 The second item
 * @return true if they are the same
 */
bool bedrock_list_compare_regular(const void *data1, const void *data2)
{
	return data1 == data2;
}

/** Add an entry to this list
 * @param list The list
 * @param data The data to add
 * @return A node pointer to the node where the data is stored
 */
bedrock_node *bedrock_list_add(bedrock_list *list, void *data)
{
	bedrock_node *node;

	bedrock_assert(list != NULL && data != NULL, return NULL);

	node = bedrock_malloc(sizeof(bedrock_node));
	bedrock_list_add_node(list, node, data);
	return node;
}

/** Add an entry to this list
 * @param list The list
 * @param node The node to use
 * @param data The data to add
 * @return A node pointer to the node where the data is stored
 */
void bedrock_list_add_node(bedrock_list *list, bedrock_node *node, void *data)
{
	bedrock_assert(list != NULL && node != NULL && data != NULL, return);

	node->data = data;

	if (list->tail)
	{
		list->tail->next = node;
		node->prev = list->tail;
		node->next = NULL;
		list->tail = node;
	}
	else
	{
		list->head = list->tail = node;
		node->prev = node->next = NULL;
	}

	list->count++;
}

/** Add an entry to this list after 'after'
 * @param list The list
 * @param node The node to add
 * @param after The node to add node after
 * @param data The data for the node
 */
void bedrock_list_add_node_after(bedrock_list *list, bedrock_node *node, bedrock_node *after, void *data)
{
	bedrock_assert(list != NULL && node != NULL && after != NULL && data != NULL, return);

	node->data = data;

	node->next = after->next;
	node->prev = after;

	if (after->next != NULL)
		after->next->prev = node;
	after->next = node;

	if (after == list->tail)
		list->tail = node;

	list->count++;
}

/** Add an entry to this list before 'before'
 * @param list The list
 * @param node The node to add
 * @param after The node to add node before
 * @param data The data for the node
 */
void bedrock_list_add_node_before(bedrock_list *list, bedrock_node *node, bedrock_node *before, void *data)
{
	bedrock_assert(list != NULL && node != NULL && before != NULL && data != NULL, return);

	node->data = data;

	node->next = before;
	node->prev = before->prev;

	if (before->prev != NULL)
		before->prev->next = node;
	before->prev = node;

	if (node->prev == list->head)
		list->head = node;
	
	list->count++;
}

/** Delete an entry from this list
 * @param list The list
 * @param data The data to remove
 * @param data A pointer to the data the node was holding, unless it was already freed
 */
void *bedrock_list_del(bedrock_list *list, const void *data)
{
	bedrock_node *n;

	bedrock_assert(list != NULL && data != NULL, return NULL);

	if (!list->compare)
		list->compare = bedrock_list_compare_regular;

	LIST_FOREACH(list, n)
	{
		if (list->compare(data, n->data))
		{
			void *data = bedrock_list_del_node(list, n);
			bedrock_free(n);
			return data;
		}
	}

	return NULL;
}

/** Remove a node from this list
 * @param list The lst
 * @param node The node
 * @param data A pointer to the data the node was holding, unless it was already freed
 */
void *bedrock_list_del_node(bedrock_list *list, bedrock_node *node)
{
	void *data = NULL;

	bedrock_assert(list != NULL && node != NULL, return NULL);

	if (node->prev)
		node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;
	if (list->head == node)
		list->head = node->next;
	if (list->tail == node)
		list->tail = node->prev;
	
	list->count--;

	if (list->free)
		list->free(node->data);
	else
		data = node->data;
	
	node->next = node->prev = NULL;
	node->data = NULL;

	return data;
}

/** Check if the list has data
 * @param list the list
 * @param data The data
 * @return true if the data exists
 */
bool bedrock_list_has_data(bedrock_list *list, const void *data)
{
	bedrock_node *n;

	bedrock_assert(list != NULL && data != NULL, return false);

	if (!list->compare)
		list->compare = bedrock_list_compare_regular;

	LIST_FOREACH(list, n)
	{
		if (list->compare(data, n->data))
		{
			return true;
		}
	}

	return false;
}

/** Clear a list
 * @param list The list
 */
void bedrock_list_clear(bedrock_list *list)
{
	bedrock_node *n, *tn;

	bedrock_assert(list != NULL, return);

	LIST_FOREACH_SAFE(list, n, tn)
	{
		bedrock_list_del_node(list, n);
		bedrock_free(n);
	}
}

