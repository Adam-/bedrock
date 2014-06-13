
#define UUID_LEN 16
#define UUID_STR_LEN 36

struct uuid
{
	unsigned char u[UUID_LEN];
};

extern void uuid_v3_from_name(struct uuid *uuid, const char *name);
extern const char *uuid_to_string(struct uuid *uuid);
