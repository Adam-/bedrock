#ifndef WIN32
#define _O_BINARY 0
#endif

extern unsigned char *bedrock_file_read(int fd, size_t *file_size);
extern bool bedrock_file_read_buf(int fd, void *dest, size_t want);

