#include "nbt/tag.h"

extern nbt_tag *nbt_parse(const unsigned char *data, size_t size);
extern void nbt_free(nbt_tag *tag);

extern nbt_tag *nbt_get(nbt_tag *tag, nbt_tag_type type, size_t size, ...);
extern void nbt_copy(nbt_tag *tag, nbt_tag_type type, void *dest, size_t dest_size, size_t size, ...);
extern void *nbt_read(nbt_tag *tag, nbt_tag_type type, size_t size, ...);
extern char *nbt_read_string(nbt_tag *tag, size_t size, ...);
extern void nbt_set(nbt_tag *tag, nbt_tag_type type, const void *src, size_t src_size, size_t size, ...);

extern void nbt_ascii_dump(nbt_tag *tag);
