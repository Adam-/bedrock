#include "util/list.h"

#define PACKET_COLUMN_BULK_INIT LIST_INIT

/* List of columns */
typedef bedrock_list packet_column_bulk;

extern void packet_column_bulk_add(struct bedrock_client *client, packet_column_bulk *columns, struct bedrock_column *column);
extern void packet_send_column_bulk(struct bedrock_client *client, packet_column_bulk *columns);

