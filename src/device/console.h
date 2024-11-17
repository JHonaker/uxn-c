#include "../common.h"
#include "../uxn.h"

#define CONSOLE_VECTOR_PORT 0x10
#define CONSOLE_READ_PORT 0x12
#define CONSOLE_TYPE_PORT 0x17
#define CONSOLE_WRITE_PORT 0x18
#define CONSOLE_ERROR_PORT 0x19
#define CONSOLE_ADDR_PORT 0x1c
#define CONSOLE_MODE_PORT 0x1e
#define CONSOLE_EXEC_PORT 0x1f

#define CONSOLE_TYPE_NO_QUEUE 0
#define CONSOLE_TYPE_STDIN 1
#define CONSOLE_TYPE_ARG 2
#define CONSOLE_TYPE_ARG_SPACER 3
#define CONSOLE_TYPE_ARG_END 4

int console_input_event(Uxn *uxn, Byte c, Byte type);
void console_deo(Uxn *uxn, Byte addr);