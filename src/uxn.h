#include "common.h"
#include "stack.h"

#ifndef uxn_h
#define uxn_h

#define T Uxn

typedef struct T {
  Byte ram[RAM_SIZE];
  Stack work;
  Stack ret;
} T;


void initUxn(T *uxn);

void Uxn_push_work(T *uxn, Byte value);
Byte Uxn_pop_work(T *uxn);
Byte Uxn_peek_work_offset(T *uxn, Byte offset);
Byte Uxn_peek_work(T *uxn);

void Uxn_push_ret(T *uxn, Byte value);
Byte Uxn_pop_ret(T *uxn);
Byte Uxn_peek_ret(T *uxn);

bool Uxn_eval(T *uxn, Short pc);

#undef T
#endif // uxn_h