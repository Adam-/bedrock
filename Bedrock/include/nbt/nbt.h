#include "nbt/tag.h"

extern nbt_tag *nbt_parse(const unsigned char *data, size_t size);
extern void nbt_free(nbt_tag *tag);

extern nbt_tag *nbt_get(nbt_tag *tag, size_t size, ...);
extern const void *nbt_read_int(nbt_tag *tag, nbt_tag_type type, size_t size, ...);
extern const char *nbt_read_string(nbt_tag *tag, size_t size, ...);

extern void nbt_ascii_dump(nbt_tag *tag);
