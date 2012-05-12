#include "nbt/tag.h"

nbt_tag *nbt_parse(const unsigned char *data, size_t size);
void nbt_free(nbt_tag *tag);
