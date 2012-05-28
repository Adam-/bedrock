#include "blocks/items.h"

enum
{
	ENTITY_EQUIPMENT_HELD
};

extern void packet_send_entity_equipment(struct bedrock_client *client, struct bedrock_client *c, uint16_t slot, struct bedrock_item *item, uint16_t damage);
