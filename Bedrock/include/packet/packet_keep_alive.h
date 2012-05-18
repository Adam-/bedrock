
extern int packet_keep_alive(struct bedrock_client *client, const unsigned char *buffer, size_t len);
extern void packet_send_keep_alive(struct bedrock_client *client, uint32_t id);
