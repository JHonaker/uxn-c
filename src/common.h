#ifndef uxn_common_h
#define uxn_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t Byte;
typedef int8_t SignedByte;
typedef uint16_t Short;
typedef int16_t SignedShort;

#define ZERO_PAGE 0x00
#define RESET_VECTOR 0x0100

#define STACK_SIZE 0x100
#define RAM_SIZE 0x10000
#define DEV_PAGE_SIZE 0x10000

#endif // uxn_common_h