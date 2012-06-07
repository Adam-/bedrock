
extern int packet_entity_animation(struct bedrock_client *client, const unsigned char *buffer, size_t len);
extern void packet_send_entity_animation(struct bedrock_client *client, struct bedrock_client *target, uint8_t anim);
