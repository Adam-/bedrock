
enum entity_metadata_type
{
	ENTITY_METADATA_TYPE_BYTE,
	ENTITY_METADATA_TYPE_SHORT,
	ENTITY_METADATA_TYPE_INT,
	ENTITY_METADATA_TYPE_FLOAT,
	ENTITY_METADATA_TYPE_STRING,
	ENTITY_METADATA_TYPE_SLOT,
	ENTITY_METADATA_TYPE_3INT
};

enum entity_metadata_index
{
	ENTITY_METADATA_INDEX_FLAGS = 0,
	ENTITY_METADATA_INDEX_DROWNING_COUNTER = 1,
	ENTITY_METADATA_INDEX_POTION_EFFECTS = 8,
	ENTITY_METADATA_INDEX_ANIMALS = 12
};

extern void packet_send_entity_metadata(struct client *client, enum entity_metadata_index index, enum entity_metadata_type type, const void *data, size_t size);
