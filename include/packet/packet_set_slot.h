
enum
{
	WINDOW_INVENTORY
};

extern void packet_send_set_slot(struct bedrock_client *client, uint8_t window_id, uint16_t slot, struct bedrock_item *item, uint8_t count, int16_t damage);
