/* Until I have a real config reader */

#define BEDROCK_WORLD_NAME "world"
#define BEDROCK_WORLD_BASE "/home/adam/world"

#define BEDROCK_LISTEN_IP "0.0.0.0"
#define BEDROCK_LISTEN_PORT 25567

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

/* Max amount of data allowed to be queued to a client before they are dropped */
#define BEDROCK_CLIENT_SENDQ_LENGTH 1024

/* Max amount of data allowed to be recieved from a client before they are dropped */
#define BEDROCK_CLIENT_RECVQ_LENGTH 1024

/* The maximum string length allowed, including the trailing \0 */
#define BEDROCK_MAX_STRING_LENGTH 120
