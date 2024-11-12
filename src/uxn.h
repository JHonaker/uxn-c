#include "common.h"

#ifndef uxn_h
#define uxn_h

#define ZERO_PAGE 0x00

#define START_PC 0x0100

#define STACK_SIZE 0x100

#define RAM_SIZE 0x10000

typedef struct {
  Byte stack[STACK_SIZE];
  Byte ptr;
} Stack;

typedef struct Uxn {
  Byte ram[RAM_SIZE];
  Stack work;
  Stack ret;
} Uxn;

void initUxn(Uxn *uxn);

void uxn_push_work(Uxn *uxn, Byte value);
Byte uxn_pop_work(Uxn *uxn);
Byte uxn_peek_work_offset(Uxn *uxn, Byte offset);
Byte uxn_peek_work(Uxn *uxn);

void uxn_push_ret(Uxn *uxn, Byte value);
Byte uxn_pop_ret(Uxn *uxn);
Byte uxn_peek_ret(Uxn *uxn);

bool uxn_eval(Uxn *uxn, Short pc);

#endif // uxn_h