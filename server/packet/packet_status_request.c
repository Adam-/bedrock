#include "server/bedrock.h"
#include "server/client.h"
#include "server/packet.h"
#include "config/config.h"
#include "packet/packet_chat_message.h"
#include "util/io.h"
#include "util/string.h"

#include <jansson.h>

int packet_status_request(struct client *client, bedrock_packet bedrock_attribute_unused *p)
{
	char bedrock_name[BEDROCK_MAX_STRING_LENGTH];
	json_t *j;
	bedrock_packet packet;
	char *c;

	snprintf(bedrock_name, sizeof(bedrock_name), "bedrock-%d.%d%s", BEDROCK_VERSION_MAJOR, BEDROCK_VERSION_MINOR, BEDROCK_VERSION_EXTRA);
	
	j = json_pack("{"
				"s: {"
					"s: s,"
					"s: i"
				"},"
				"s: {"
					"s: i,"
					"s: i"
				"},"
				"s: {"
					"s: s"
				"}"
			"}",
				"version",
					"name", bedrock_name,
					"protocol", BEDROCK_PROTOCOL_VERSION,
				"players",
					"max", server_maxusers,
					"online", authenticated_client_count,
				"description",
					"text", server_desc);

	c = json_dumps(j, 0);

	packet_init(&packet, STATUS_SERVER_RESPONSE);
	packet_pack_string(&packet, c);
	client_send_packet(client, &packet);

	free(c);
	json_decref(j);

	return ERROR_OK;
}
