
extern void crypto_init();
extern void crypto_shutdown();
extern int crypto_pubkey_len();
extern unsigned char *crypto_pubkey();
extern int crypto_rsa_decrypt(const unsigned char *src, size_t src_len, unsigned char *dest, size_t dest_len);

