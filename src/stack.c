#include <stdio.h>
#include <stdlib.h>

#include "stack.h"

struct Stack {
  Byte data[STACK_SIZE];
  Byte ptr;
};

/**
 * Pointer movement with circular bounds.
 */

void increment_ptr(Stack *stack) { stack->ptr = (stack->ptr + 1) % STACK_SIZE; }

void decrement_ptr(Stack *stack) { stack->ptr = (stack->ptr - 1) % STACK_SIZE; }

void Stack_zero(Stack *stack) {
  for (int i = 0; i < 0x100; i++) {
    stack->data[i] = 0;
  }
  stack->ptr = 0;
}

/**
 * Lifecycle management.
 */

void Stack_init(Stack *stack) {
  if (stack) {
    *stack = (Stack){.ptr = 0, .data = {0}};
  }
}

void Stack_destroy(Stack *stack) {
  if (stack) {
    Stack_zero(stack);
  }
}

Stack *Stack_new() {
  Stack *stack = (Stack *)malloc(sizeof(Stack));
  Stack_init(stack);
  return stack;
}

void Stack_delete(Stack *stack) {
  if (stack) {
    Stack_destroy(stack);
    free(stack);
  }
}

void Stack_push(Stack *stack, Byte value) {
  stack->data[stack->ptr] = value;
  increment_ptr(stack);
}

Byte Stack_pop(Stack *stack) {
  decrement_ptr(stack);
  return stack->data[stack->ptr];
}

Byte Stack_peek_offset(Stack *stack, Byte offset) {
  Byte index = (stack->ptr - (offset + 1)) % STACK_SIZE;
  return stack->data[index];
}

Byte Stack_peek(Stack *stack) { return Stack_peek_offset(stack, 0); }

void Stack_set_ptr(Stack *stack, Byte ptr) { stack->ptr = ptr; }

Byte Stack_get_ptr(Stack *stack) { return stack->ptr; }

void Stack_dump(Stack *stack, const char *name) {

  const int print_depth = 8;
  printf("%s ", name);

  for (int i = stack->ptr - print_depth; i < stack->ptr; i++) {
    printf("%02x", stack->data[i % STACK_SIZE]);
    printf(i == -1 ? "|" : " ");
  }
  printf("<\n");
}
