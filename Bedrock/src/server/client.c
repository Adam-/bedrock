#include "server/client.h"
#include "util/memory.h"

bedrock_list client_list;

bedrock_client *bedrock_client_create()
{
	bedrock_client *client = bedrock_malloc(sizeof(bedrock_client));
	bedrock_list_add(&client_list, client);
	return client;
}

void bedrock_client_free(bedrock_client *client)
{
	if (client->fd.open)
		bedrock_fd_close(&client->fd);

	bedrock_list_del(&client_list, client);

	bedrock_free(client);
}

void bedrock_client_read(bedrock_fd *fd)
{
}

void bedrock_client_write(bedrock_fd *fd)
{
}
