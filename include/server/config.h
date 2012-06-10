/* Until I have a real config reader */

#define BEDROCK_WORLD_NAME "world"
#define BEDROCK_WORLD_BASE "/home/adam/world"

#define BEDROCK_DESCRIPTION "Adam's custom server"
#define BEDROCK_MAX_USERS 8

#define BEDROCK_LISTEN_IP "0.0.0.0"
#define BEDROCK_LISTEN_PORT 25565

/* Tick length in milliseconds */
#define BEDROCK_TICK_LENGTH 50
/* Day length, in ticks */
#define BEDROCK_DAY_LENGTH 24000

/* Min username length */
#define BEDROCK_USERNAME_MIN 2
/* Max username length (with trailing \0), is there a standard for this? */
#define BEDROCK_USERNAME_MAX 17

/* The number of chunks around the player to send to them */
#define BEDROCK_VIEW_LENGTH 10

/* Protocol version we support */
#define BEDROCK_PROTOCOL_VERSION 29

/* Size of the initial buffer for clients */
#define BEDROCK_CLIENT_SEND_SIZE 4096

/* Max amount of data allowed to be recieved from a client before they are dropped */
#define BEDROCK_CLIENT_RECVQ_LENGTH 1024

/* The maximum string length allowed, including the trailing \0 */
#define BEDROCK_MAX_STRING_LENGTH 120

/* Number of blocks on one side of a chunk. Cube to get the number of blocks actually in a chunk */
#define BEDROCK_BLOCKS_PER_CHUNK   16
/* The number of chunks per column */
#define BEDROCK_CHUNKS_PER_COLUMN  16
/* The number of columns on the side of a region. Square for (max) columns per region. */
#define BEDROCK_COLUMNS_PER_REGION 32

#define BEDROCK_BLOCK_LENGTH 4096
#define BEDROCK_DATA_LENGTH 2048
#define BEDROCK_BIOME_LENGTH 256
