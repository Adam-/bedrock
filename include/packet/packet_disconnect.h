
extern int packet_disconnect(struct bedrock_client *client, const unsigned char *buffer, size_t len);
extern void packet_send_disconnect(struct bedrock_client *client, const char *reason);
