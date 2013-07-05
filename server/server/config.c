#include "server/bedrock.h"
#include "server/world.h"
#include "server/oper.h"
#include "config/hard.h"
#include "util/yml.h"
#include "util/memory.h"
#include "util/string.h"

char server_desc[BEDROCK_MAX_STRING_LENGTH];
int server_maxusers;
char server_ip[64];
int server_port;
uint16_t bedrock_conf_log_level = 0;
bool allow_new_users = false;

int config_parse(const char *config)
{
	bedrock_node *node;
	struct yaml_object *object = yaml_parse(config);
	if (object == NULL)
		return -1;
	
	LIST_FOREACH(&object->objects, node)
	{
		struct yaml_object *o = node->data;

		if (!strcmp(o->name, "world"))
		{
			bedrock_node *node2;
			
			LIST_FOREACH(&o->objects, node2)
			{
				struct yaml_object *w = node2->data;
				bedrock_node *node3;
				struct world *world = bedrock_malloc(sizeof(struct world));

				LIST_FOREACH(&w->objects, node3)
				{
					struct yaml_object *attr = node3->data;

					if (!strcmp(attr->name, "name"))
						strncpy(world->name, attr->value, sizeof(world->name));
					else if (!strcmp(attr->name, "path"))
						strncpy(world->path, attr->value, sizeof(world->path));
				}

				bedrock_list_add(&world_list, world);

				break; /* Only one world */
			}
		}
		else if (!strcmp(o->name, "server"))
		{
			bedrock_node *node2;

			LIST_FOREACH(&o->objects, node2)
			{
				struct yaml_object *attr = node2->data;

				if (!strcmp(attr->name, "description"))
					strncpy(server_desc, attr->value, sizeof(server_desc));
				else if (!strcmp(attr->name, "maxusers"))
					server_maxusers = atoi(attr->value);
				else if (!strcmp(attr->name, "ip"))
					strncpy(server_ip, attr->value, sizeof(server_ip));
				else if (!strcmp(attr->name, "port"))
					server_port = atoi(attr->value);
				else if (!strcmp(attr->name, "log_level"))
				{
					bedrock_node *node3;
					
					LIST_FOREACH(&attr->objects, node3)
					{
						struct yaml_object *flag = node3->data;

						if (!strcmp(flag->value, "CRIT"))
							bedrock_conf_log_level |= LEVEL_CRIT;
						if (!strcmp(flag->value, "WARN"))
							bedrock_conf_log_level |= LEVEL_WARN;
						if (!strcmp(flag->value, "INFO"))
							bedrock_conf_log_level |= LEVEL_INFO;
						if (!strcmp(flag->value, "DEBUG"))
							bedrock_conf_log_level |= LEVEL_DEBUG;
						if (!strcmp(flag->value, "COLUMN"))
							bedrock_conf_log_level |= LEVEL_COLUMN;
						if (!strcmp(flag->value, "NBT_DEBUG"))
							bedrock_conf_log_level |= LEVEL_NBT_DEBUG;
						if (!strcmp(flag->value, "THREAD"))
							bedrock_conf_log_level |= LEVEL_THREAD;
						if (!strcmp(flag->value, "BUFFER"))
							bedrock_conf_log_level |= LEVEL_BUFFER;
						if (!strcmp(flag->value, "IO_DEBUG"))
							bedrock_conf_log_level |= LEVEL_IO_DEBUG;
						if (!strcmp(flag->value, "PACKET_DEBUG"))
							bedrock_conf_log_level |= LEVEL_PACKET_DEBUG;
					}
				}
				else if (!strcmp(attr->name, "allow_new_users"))
					allow_new_users = !strcmp(attr->value, "true");
			}
		}
		else if (!strcmp(o->name, "oper"))
		{
			bedrock_node *node2;

			LIST_FOREACH(&o->objects, node2)
			{
				struct yaml_object *o = node2->data;
				bedrock_node *node3;
				struct oper *oper = bedrock_malloc(sizeof(struct oper));

				oper->commands.free = bedrock_free;

				LIST_FOREACH(&o->objects, node3)
				{
					struct yaml_object *attr = node3->data;

					if (!strcmp(attr->name, "name"))
						strncpy(oper->username, attr->value, sizeof(oper->username));
					else if (!strcmp(attr->name, "password"))
						strncpy(oper->password, attr->value, sizeof(oper->password));
					else if (!strcmp(attr->name, "commands"))
					{
						bedrock_node *node4;

						LIST_FOREACH(&attr->objects, node4)
						{
							struct yaml_object *cmd = node4->data;
							bedrock_list_add(&oper->commands, bedrock_strdup(cmd->value));
						}
					}
				}

				oper_conf_list.free = (bedrock_free_func) oper_free;
				bedrock_list_add(&oper_conf_list, oper);
			}
		}
	}

	yaml_object_free(object);

	return 0;
}

