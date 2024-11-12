#include "common.h"

#ifndef uxn_h
#define uxn_h

typedef struct {
  Byte stack[0x100];
  Byte ptr;
} Stack;

typedef struct Uxn {
  Byte ram[0x10000];
  Stack work;
  Stack ret;
} Uxn;

#endif // uxn_h