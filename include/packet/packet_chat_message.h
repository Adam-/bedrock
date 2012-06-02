
#define SPECIAL_CHAR 0xA7

enum
{
	COLOR_BLACK        = 0x30, // 0
	COLOR_DARK_BLUE    = 0x31, // 1
	COLOR_DARK_GREEN   = 0x32, // 2
	COLOR_DARK_CYAN    = 0x33, // 3
	COLOR_DARK_RED     = 0x34, // 4
	COLOR_PURPLE       = 0x35, // 5
	COLOR_GOLD         = 0x36, // 6
	COLOR_GREY         = 0x37, // 7
	COLOR_DARK_GREY    = 0x38, // 8
	COLOR_BLUE         = 0x39, // 9
	COLOR_BRIGHT_GREEN = 0x61, // a
	COLOR_CYAN         = 0x62, // b
	COLOR_RED          = 0x63, // c
	COLOR_PINK         = 0x64, // d
	COLOR_YELLOW       = 0x65, // e
	COLOR_WHITE        = 0x66  // f
};

enum
{
	STYLE_RANDOM        = 0x6B, // k
	STYLE_BOLD          = 0x6C, // l
	STYLE_STRIKETHROUGH = 0x6D, // m
	STYLE_UNDERLINED    = 0x6E, // n
	STYLE_ITALIC        = 0x6F, // o
	STYLE_PLAIN         = 0x72  // r
};

extern int packet_chat_message(struct bedrock_client *client, const unsigned char *buffer, size_t len);
extern void packet_send_chat_message(struct bedrock_client *client, const char *buf, ...);
