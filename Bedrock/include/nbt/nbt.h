#include "nbt/tag.h"

extern nbt_tag *nbt_parse(const unsigned char *data, size_t size);
extern void nbt_free(nbt_tag *tag);
extern void nbt_ascii_dump(nbt_tag *tag);
