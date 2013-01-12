#include "blocks/items.h"

enum
{
	ENTITY_EQUIPMENT_HELD
};

extern void packet_send_entity_equipment(struct client *client, struct client *c, uint16_t slot, struct item *item, uint16_t damage);
