#include "server/bedrock.h"
#include "util/memory.h"
#include "config/hard.h"

#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/x509.h>

static RSA *keypair;
static unsigned char *pubkey_encoded;
static int pubkey_len;

void crypto_init()
{
	keypair = RSA_generate_key(BEDROCK_CERT_BIT_SIZE, RSA_F4, NULL, NULL);
	if (keypair == NULL)
	{
		unsigned long en = ERR_get_error();
		bedrock_log(LEVEL_CRIT, "cert: Unable to generate RSA keypair - %s", ERR_error_string(en, NULL));
		exit(-1);
	}

	pubkey_len = i2d_RSA_PUBKEY(keypair, &pubkey_encoded);
	if (pubkey_len <= 0)
	{
		unsigned long en = ERR_get_error();
		bedrock_log(LEVEL_CRIT, "cert: Unable to encode public key - %s", ERR_error_string(en, NULL));
		exit(-1);
	}
}

void crypto_shutdown()
{
	free(pubkey_encoded);
	RSA_free(keypair);
}

int crypto_pubkey_len()
{
	return pubkey_len;
}

unsigned char *crypto_pubkey()
{
	return pubkey_encoded;
}

