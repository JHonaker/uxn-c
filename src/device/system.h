#include "../common.h"
#include "../uxn.h"

#define SYSTEM_EXPANSION_PORT 0x02
#define SYSTEM_WST_PORT 0x04
#define SYSTEM_RST_PORT 0x05
#define SYSTEM_METADATA_PORT 0x06
#define SYSTEM_RED_PORT 0x08
#define SYSTEM_GREEN_PORT 0x0a
#define SYSTEM_BLUE_PORT 0x0c
#define SYSTEM_DEBUG_PORT 0x0e
#define SYSTEM_STATE_PORT 0x0f

typedef enum ExpansionOp { FILL, CPYL, CPYR } ExpansionOp;

void system_reboot(Uxn *uxn, char *rom_path, bool soft_reboot);
void system_inspect(Uxn *uxn);
int system_error(char *msg, const char *err);
int system_boot(Uxn *uxn, char *rom_path);

Byte system_dei(Uxn *uxn, Byte addr);
void system_deo(Uxn *uxn, Byte addr);