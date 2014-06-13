#include "server/physics.h"
#include "server/column.h"
#include "blocks/blocks.h"

static bedrock_list projectiles;

void physics_add(struct column *column, struct projectile *p)
{
	bedrock_log(LEVEL_DEBUG, "Adding projectile %d to world %s", p->id, column->region->world->name);

	bedrock_assert(!p->moving, return);
	p->moving = true;

	p->world = column->region->world;
	p->column = column;

	bedrock_list_add_node(&projectiles, &p->node, p);
	bedrock_list_add_node(&column->projectiles, &p->cnode, p);
}

void physics_remove(struct projectile *p)
{
	bedrock_assert(p->moving, return);
	bedrock_list_del_node(&projectiles, &p->node);
	bedrock_list_del_node(&p->column->projectiles, &p->cnode);
	p->moving = false;
}

static void apply_drag(float *d, float drag)
{
	if (*d < 0)
	{
		*d += drag;
		if (*d > 0)
			*d = 0;
	}
	else if (*d > 0)
	{
		*d -= drag;
		if (*d < 0)
			*d = 0;
	}
}

void physics_process(long ticks)
{
	bedrock_node *node, *node2;

	LIST_FOREACH_SAFE(&projectiles, node, node2)
	{
		struct projectile *p = node->data;
		struct column *column;
		struct chunk *chunk;
		uint8_t *b;

		/* apply drag, which moves the x/y/z values of the velocity twords 0 */
		apply_drag(&p->velocity.x, p->drag * ticks);
		apply_drag(&p->velocity.y, p->drag * ticks);
		apply_drag(&p->velocity.z, p->drag * ticks);

		/* apply acceleration due to gravity, if the current accleration is less than the terminal velocity, which is usually negative */
		if (p->velocity.y > p->terminal_velocity)
		{
			p->velocity.y += p->ag * ticks;
			/* Don't allow accelerating due to gravity faster than the terminal velocity */
			if (p->velocity.y <= p->terminal_velocity)
				p->velocity.y = p->terminal_velocity;
		}

		/* finally move projectile according to velocity */
		p->pos.x += p->velocity.x * ticks;
		p->pos.y += p->velocity.y * ticks;
		p->pos.z += p->velocity.z * ticks;

		bedrock_log(LEVEL_DEBUG, "physics: Projectile %d moves to %f, %f, %f with velocity %f, %f, %f", p->id, p->pos.x, p->pos.y, p->pos.z, p->velocity.x, p->velocity.y, p->velocity.z);

		if (p->pos.y < 0 || p->pos.y >= BEDROCK_MAX_HEIGHT)
		{
			bedrock_log(LEVEL_DEBUG, "physics: Projectile %d falls off of the world", p->id);

			/* Projectile falls off of the world */
			p->on_destroy(p, p->data);

			physics_remove(p);
			continue;
		}
		
		column = find_column_from_world_which_contains(p->world, p->pos.x, p->pos.z);
		if (column == NULL)
		{
			/* Projectile goes off of the side of the world, we'll let it fall, and remove it above if it reaches y=0 */
			continue;
		}

		if (p->column != column)
		{
			/* Projectile moves across columns */
			p->on_move_column(p, p->column, column, p->data);
			p->column = column;
		}

		p->on_update(p, p->data);

		chunk = find_chunk_from_column_which_contains(column, p->pos.x, p->pos.y, p->pos.z);
		if (chunk == NULL)
		{
			/* Flying through a chunk of air, fine */
			continue;
		}

		b = chunk_get_block(chunk, p->pos.x, p->pos.y, p->pos.z);
		if (b != NULL && *b != BLOCK_AIR)
		{
			uint8_t *b2;

			/* Collision! */

			/* Move back until we are outside of whatever we're in */
			while ((b2 = chunk_get_block_from_world(p->world, p->pos.x, p->pos.y, p->pos.z)) && *b2 != BLOCK_AIR)
			{
				p->pos.x -= p->velocity.x;
				p->pos.y -= p->velocity.y;
				p->pos.z -= p->velocity.z;
			}

			p->on_update(p, p->data);

			/* x and z velocity go to 0 */
			p->velocity.x = 0;
			p->velocity.z = 0;

			/* still falling,but vertically? */
			if (p->pos.y > 0)
				if (!(b2 = chunk_get_block_from_world(p->world, p->pos.x, p->pos.y - 1, p->pos.z)) || *b2 == BLOCK_AIR)
					continue;

			p->on_collide(p, b, p->data);

			physics_remove(p);
			continue;
		}
	}
}

