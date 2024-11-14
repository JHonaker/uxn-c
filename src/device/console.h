#include "../common.h"
#include "../uxn.h"

#define CONSOLE_ADDR_VECTOR 0x10
#define CONSOLE_ADDR_READ 0x12
#define CONSOLE_ADDR_TYPE 0x17
#define CONSOLE_ADDR_WRITE 0x18
#define CONSOLE_ADDR_ERROR 0x19
#define CONSOLE_ADDR_ADDR 0x1c
#define CONSOLE_ADDR_MODE 0x1e
#define CONSOLE_ADDR_EXEC 0x1f

#define CONSOLE_TYPE_NO_QUEUE 0
#define CONSOLE_TYPE_STDIN 1
#define CONSOLE_TYPE_ARG 2
#define CONSOLE_TYPE_ARG_SPACER 3
#define CONSOLE_TYPE_ARG_END 4

void console_deo(Uxn* uxn, Byte addr);