
#define UUID_LEN 16
#define UUID_STR_LEN 36

struct uuid
{
	unsigned char u[UUID_LEN];
};

extern void uuid_v3_generate(struct uuid *uuid);
extern const char *uuid_to_string(struct uuid *uuid);
