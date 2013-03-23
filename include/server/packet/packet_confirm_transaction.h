
extern int packet_confirm_transaction(struct client *client, const bedrock_packet *p);
extern void packet_send_confirm_transaction(struct client *client, uint8_t window, int16_t action_number, uint8_t accepted);

