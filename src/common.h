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
#define RAM_PAGES 0x10
#define RAM_PAGE_SIZE 0x10000
#define DEV_PAGE_SIZE 0x10000

#define DEVICE_PAGE_SYSTEM 0x00
#define DEVICE_PAGE_CONSOLE 0x10
#define DEVICE_PAGE_SCREEN 0x20
#define DEVICE_PAGE_AUDIO1 0x30
#define DEVICE_PAGE_AUDIO2 0x40
#define DEVICE_PAGE_AUDIO3 0x50
#define DEVICE_PAGE_AUDIO4 0x60
// No device at 0x70
#define DEVICE_PAGE_CONTROLLER 0x80
#define DEVICE_PAGE_MOUSE 0x90
#define DEVICE_PAGE_FILE1 0xa0
#define DEVICE_PAGE_FILE2 0xb0
#define DEVICE_PAGE_DATETIME 0xc0
// No device at 0xd0
// No device at 0xe0
// No device at 0xf0

#endif // uxn_common_h