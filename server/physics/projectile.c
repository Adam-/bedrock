#include "server/physics.h"
#include "server/column.h"
#include "server/client.h"

void projectile_update(struct projectile *p, void *data)
{
	bedrock_node *node;

	/* Update projectile to all players */

	LIST_FOREACH(&p->column->players, node)
	{
		struct client *c = node->data;
		packet_send_entity_teleport_projectile(c, p);
	}
}

void projectile_move_column(struct projectile *p, struct column *oldc, struct column *newc, void *data)
{
	bedrock_node *node;

	/* swap columns for this projectile */
	bedrock_list_del_node(&oldc->projectiles, &p->cnode);
	bedrock_list_add_node(&newc->projectiles, &p->cnode, p);

	/* Projectile moves across columns, and potentially in or out of render distance of clients */
	LIST_FOREACH(&newc->players, node)
	{
		struct client *c = node->data;

		if (!bedrock_list_has_data(&c->columns, oldc))
		{
			/* This projectile is new and is now within this clients render distance */
			p->on_new_client(p, c, p->data);
		}
	}

	LIST_FOREACH(&oldc->players, node)
	{
		struct client *c = node->data;

		/* PLayers who can see this column, but not the new one, have the projectile go away */
		if (!bedrock_list_has_data(&c->columns, newc))
		{
			packet_send_destroy_entity_projectile(c, p);
		}
	}
}

void projectile_collide(struct projectile *p, uint8_t *block, void *data)
{
}

void projectile_on_new_client(struct projectile *p, struct client *client, void *data)
{
}

void projectile_destroy(struct projectile *p, void *data)
{
	bedrock_node *node;

	LIST_FOREACH(&p->column->players, node)
	{
		struct client *c = node->data;
		packet_send_destroy_entity_projectile(c, p);
	}
}

