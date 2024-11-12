#ifndef uxn_common_h
#define uxn_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint8_t Byte;
typedef uint16_t Short;

#define ZERO_PAGE 0x00
#define START_PC 0x0100

#define STACK_SIZE 0x100
#define RAM_SIZE 0x10000


#endif // uxn_common_h