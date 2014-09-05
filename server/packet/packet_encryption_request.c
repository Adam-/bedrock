#include "server/client.h"
#include "server/packet.h"
#include "server/packets.h"
#include "util/crypto.h"

void packet_send_encryption_request(struct client *client)
{
	int16_t pubkey_len = crypto_pubkey_len();
	int16_t verify_token_length = BEDROCK_VERIFY_TOKEN_LEN;
	bedrock_packet packet;

	packet_init(&packet, LOGIN_SERVER_ENCRYPTION_REQUEST);

	packet_pack_string(&packet, "-"); // server ID

	packet_pack_varint(&packet, pubkey_len);
	packet_pack(&packet, crypto_pubkey(), pubkey_len);

	packet_pack_varint(&packet, verify_token_length);
	packet_pack(&packet, crypto_auth_token(), BEDROCK_VERIFY_TOKEN_LEN);

	client_send_packet(client, &packet);
}
