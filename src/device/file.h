#include "../common.h"
#include "../uxn.h"

#define FILE_A_PAGE 0xa0
#define FILE_B_PAGE 0xb0
#define FILE_VECTOR_PORT 0x00
#define FILE_SUCCESS_PORT 0x02
#define FILE_STAT_PORT 0x04
#define FILE_DELETE_PORT 0x06
#define FILE_APPEND_PORT 0x07
#define FILE_NAME_PORT 0x08
#define FILE_LENGTH_PORT 0x0a
#define FILE_READ_PORT 0x0c
#define FILE_WRITE_PORT 0x0e

void file_deo(Uxn *uxn, Byte addr);
Byte file_dei(Uxn *uxn, Byte addr);