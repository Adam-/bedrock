#include "server/client.h"
#include "server/packet.h"
#include "util/crypto.h"

void packet_send_encryption_request(struct bedrock_client *client)
{
	int i;
	int16_t pubkey_len = crypto_pubkey_len();
	unsigned char verify_token[BEDROCK_VERIFY_TOKEN_LEN];
	int16_t verify_token_length = BEDROCK_VERIFY_TOKEN_LEN;
	bedrock_packet packet;

	for (i = 0; i < BEDROCK_VERIFY_TOKEN_LEN; ++i)
		verify_token[i] = rand() % 0xFF;

	packet_init(&packet, ENCRYPTION_REQUEST);

	packet_pack_header(&packet, ENCRYPTION_REQUEST);
	packet_pack_string(&packet, "-"); // server ID

	packet_pack_int(&packet, &pubkey_len, sizeof(pubkey_len));
	packet_pack(&packet, crypto_pubkey(), pubkey_len);

	packet_pack_int(&packet, &verify_token_length, sizeof(verify_token_length));
	packet_pack(&packet, verify_token, sizeof(verify_token));

	client_send_packet(client, &packet);

	memcpy(client->key, verify_token, sizeof(verify_token));
}
