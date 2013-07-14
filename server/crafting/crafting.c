#include "blocks/items.h"
#include "crafting/recipe.h"

static bool recipe_compare(struct item_stack recipe[3][3], struct item_stack match[3][3])
{
	int i, j;
	for (i = 0; i < 3; ++i)
		for (j = 0; j < 3; ++j)
			if (recipe[i][j].id != match[i][j].id)
				return false;
	return true;
}

static bool recipe_matches(struct item_stack recipe[3][3], struct item_stack match[3][3])
{
	static struct item_stack empty[3][3];
	if (recipe_compare(recipe, empty))
		return false;

	if (recipe_compare(recipe, match))
		return true;

	/* Shift right */
	if (recipe[0][2].id == 0 && recipe[1][2].id == 0 && recipe[2][2].id == 0)
	{
		struct item_stack right[3][3];
		int i;

		memcpy(right, recipe, sizeof(right));

		for (i = 0; i < 3; ++i)
		{
			right[i][2] = right[i][1];
			right[i][1] = right[i][0];
			memset(&right[i][0], 0, sizeof(right[i][0]));
		}

		if (recipe_matches(right, match))
			return true;
	}

	/* Shift down */
	if (recipe[2][0].id == 0 && recipe[2][1].id == 0 && recipe[2][2].id == 0)
	{
		struct item_stack down[3][3];

		memcpy(down, recipe, sizeof(down));

		memmove(down[2], down[1], sizeof(struct item_stack) * 3);
		memmove(down[1], down[0], sizeof(struct item_stack) * 3);
		memset(down[0], 0, sizeof(struct item_stack) * 3);

		if (recipe_matches(down, match))
			return true;
	}

	return false;
}

void crafting_process(struct item_stack *in, size_t length, struct item_stack *output)
{
	struct item_stack input[3][3];
	bedrock_node *node;

	output->id = 0;
	output->count = 0;
	output->metadata = 0;

	bedrock_assert(length == 4 || length == 9, return);

	if (length == 9)
		memcpy(input, in, sizeof(input));
	else if (length == 4)
	{
		memset(input, 0, sizeof(input));
		memcpy(input[0], in, sizeof(struct item_stack) * 2);
		memcpy(input[1], in + 2, sizeof(struct item_stack) * 2);
	}
	else
		return;

	LIST_FOREACH(&recipies, node)
	{
		struct recipe *recipe = node->data;

		if (recipe_matches(recipe->input, input))
		{
			*output = recipe->output;
			return;
		}
	}
}

void crafting_remove_one(struct item_stack *in, size_t length)
{
	size_t i;

	for (i = 0; i < length; ++i)
	{
		struct item_stack *stack = &in[i];

		if (stack->count)
		{
			if (!--stack->count)
			{
				stack->id = 0;
				stack->metadata = 0;
			}
		}
	}
}

