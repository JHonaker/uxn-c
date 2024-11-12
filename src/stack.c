#include "stack.h"

/**
 * Sets the stack and its pointer to zero.
 * 
 * @param stack Pointer to the stack.
 */
void Stack_zero(Stack *stack) {
  for (int i = 0; i < 0x100; i++) {
    stack->data[i] = 0;
  }
  stack->ptr = 0;
}

void increment_ptr(Stack *stack) {
  stack->ptr = (stack->ptr + 1) % STACK_SIZE;
}

void decrement_ptr(Stack *stack) {
  stack->ptr = (stack->ptr - 1) % STACK_SIZE;
}

/**
 * Initializes the stack.
 * 
 * @param stack Pointer to the uninitialized stack.
 */
void Stack_init(Stack *stack) {
  Stack_zero(stack);
}

/**
 * Pushes a value onto the stack.
 *
 * @param stack Pointer to the stack.
 * @param value The byte value to be pushed onto the stack.
 */
void Stack_push(Stack *stack, Byte value) {
  stack->data[stack->ptr] = value;
  increment_ptr(stack);
}

// /**
//  * Pops a value from the stack and returns it.
//  *
//  * @param stack Pointer to the stack.
//  * 
//  * @return The byte value popped from the stack.
//  */
Byte Stack_pop(Stack *stack) {
  decrement_ptr(stack);
  return stack->data[stack->ptr];
}

// /**
//  * Peeks at a byte in the stack at a given offset from the top.
//  * 
//  * @param stack Pointer to the stack.
//  * @param offset The offset from the top of the stack. 0 is the top of the stack.
//  *
//  * @return The byte value at the specified offset in the stack.
//  */
Byte Stack_peek_offset(Stack *stack, Byte offset) {
  Byte index = (stack->ptr - (offset + 1)) % STACK_SIZE;
  return stack->data[index];
}

// /**
//  * Peeks at the top value in the stack without removing it.
//  * 
//  * @param stack Pointer to the stack.
//  * 
//  * @return The byte value at the top of the stack.
//  */
Byte Stack_peek(Stack *stack) {
  return Stack_peek_offset(stack, 0);
}