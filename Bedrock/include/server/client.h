#include "util/fd.h"
#include "util/list.h"

typedef struct
{
	bedrock_fd fd;
} bedrock_client;

extern bedrock_list client_list;

extern bedrock_client *bedrock_client_create();
extern void bedrock_client_free(bedrock_client *client);

extern void bedrock_client_read(bedrock_fd *fd);
extern void bedrock_client_write(bedrock_fd *fd);
