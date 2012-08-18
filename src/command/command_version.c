#include "server/bedrock.h"
#include "server/command.h"

void command_version(struct bedrock_command_source *source, int bedrock_attribute_unused argc, const char bedrock_attribute_unused **argv)
{
	command_reply(source, "Bedrock version %d.%d%s, built on %s at %s.", BEDROCK_VERSION_MAJOR, BEDROCK_VERSION_MINOR, BEDROCK_VERSION_EXTRA, __DATE__, __TIME__);
}
