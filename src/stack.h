#ifndef STACK_H
#define STACK_H

#include "common.h"

#define T Stack

typedef struct T {
  Byte data[STACK_SIZE];
  Byte ptr;
} T;

void Stack_init(T *stack);
void Stack_zero(T *stack);
void Stack_push(T *stack, Byte value);
Byte Stack_pop(T *stack);
Byte Stack_peek_offset(T *stack, Byte offset);
Byte Stack_peek(T *stack);


#undef T
#endif // STACK_H