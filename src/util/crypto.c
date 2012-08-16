#include "server/bedrock.h"
#include "util/memory.h"
#include "config/hard.h"

#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/x509.h>

static RSA *keypair;
static unsigned char *pubkey_encoded;
static int pubkey_len;
static unsigned char auth_token[BEDROCK_VERIFY_TOKEN_LEN]; /* Contains our authentication token for handshake */

void crypto_init()
{
	int i;

	keypair = RSA_generate_key(BEDROCK_CERT_BIT_SIZE, RSA_F4, NULL, NULL);
	if (keypair == NULL)
	{
		unsigned long en = ERR_get_error();
		bedrock_log(LEVEL_CRIT, "crypto: Unable to generate RSA keypair - %s", ERR_error_string(en, NULL));
		exit(-1);
	}

	pubkey_len = i2d_RSA_PUBKEY(keypair, &pubkey_encoded);
	if (pubkey_len <= 0)
	{
		unsigned long en = ERR_get_error();
		bedrock_log(LEVEL_CRIT, "crypto: Unable to encode public key - %s", ERR_error_string(en, NULL));
		exit(-1);
	}

	/* Generate authentication token */
	for (i = 0; i < BEDROCK_VERIFY_TOKEN_LEN; ++i)
		auth_token[i] = rand() % 0xFF;
}

void crypto_shutdown()
{
	OPENSSL_free(pubkey_encoded);
	RSA_free(keypair);
	CRYPTO_cleanup_all_ex_data();
}

int crypto_pubkey_len()
{
	return pubkey_len;
}

unsigned char *crypto_pubkey()
{
	return pubkey_encoded;
}

unsigned char *crypto_auth_token()
{
	return auth_token;
}

const EVP_CIPHER *crypto_cipher()
{
	return EVP_aes_128_cfb8();
}

int crypto_rsa_decrypt(const unsigned char *src, size_t src_len, unsigned char *dest, size_t dest_len)
{
	int len;

	bedrock_assert(dest_len >= (size_t) RSA_size(keypair), ;);

	len = RSA_private_decrypt(src_len, src, dest, keypair, RSA_PKCS1_PADDING);
	if (len == -1)
	{
		unsigned long en = ERR_get_error();
		bedrock_log(LEVEL_CRIT, "crypto: Unable to decrypt data with RSA keypair - %s", ERR_error_string(en, NULL));
		len = 0;
	}

	return len;
}

int crypto_aes_encrypt(EVP_CIPHER_CTX *context, const unsigned char *src, size_t src_len, unsigned char *dest, size_t dest_len)
{
	int out_len = dest_len, final_len = 0;

	if (!EVP_EncryptUpdate(context, dest, &out_len, src, src_len))
		bedrock_log(LEVEL_CRIT, "crypto: Unable to update encryption context");
	if (!EVP_EncryptFinal_ex(context, dest + out_len, &final_len))
		bedrock_log(LEVEL_CRIT, "crypto: Unable to finalize encryption context");
	
	bedrock_assert((size_t) (out_len + final_len) <= dest_len, ;);

	return out_len + final_len;
}

int crypto_aes_decrypt(EVP_CIPHER_CTX *context, const unsigned char *src, size_t src_len, unsigned char *dest, size_t dest_len)
{
	int out_len = dest_len, final_len = 0;

	if (!EVP_DecryptUpdate(context, dest, &out_len, src, src_len))
		bedrock_log(LEVEL_CRIT, "crypto: Unable to update decryption context");
	if (!EVP_DecryptFinal_ex(context, dest + out_len, &final_len))
		bedrock_log(LEVEL_CRIT, "crypto: Unable to finalize decryption context");

	bedrock_assert((size_t) (out_len + final_len) <= dest_len, ;);

	return out_len + final_len;
}

