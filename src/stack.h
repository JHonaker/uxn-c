#ifndef STACK_H
#define STACK_H

#include "common.h"

#define T Stack

typedef struct T T;

// Lifetime management

/**
 * @brief Initialize the stack.
 * 
 * Only use this function on an uninitialized stack.
 * 
 * Each stack initialized with this function must be destroyed with a call to `Stack_destroy`.
 * 
 * @param stack Pointer to the uninitialized stack.
 */
void Stack_init(T* stack);

/**
 * @brief Destroy the stack.
 * 
 * Only use this function on a stack initialized with `Stack_init`.
 * 
 * @param stack Pointer to the stack.
 */
void Stack_destroy(T* stack);

/**
 * @brief Allocate and initialize a new stack.
 * 
 * The stack must be destroyed with a call to `Stack_delete`.
 * 
 * @return Pointer to the new stack.
 */
T* Stack_new();

/**
 * @brief Destroy and deallocate a stack.
 * 
 * Only use this function on a stack allocated with `Stack_new`.
 * 
 * @param stack Pointer to the stack.
 */
void Stack_delete(T* stack);

// Stack operations

/**
 * @brief Set the stack and its pointer to zero.
 * 
 * @param stack Pointer to the stack.
 */
void Stack_zero(T* stack);

/**
 * @brief Push a value onto the stack.
 * 
 * @param stack Pointer to the stack.
 * @param value The byte value to be pushed onto the stack.
 */
void Stack_push(T* stack, Byte value);

/**
 * @brief Pop a value from the stack and return it.
 * 
 * @param stack Pointer to the stack.
 * @return The byte value popped from the stack.
 */
Byte Stack_pop(T* stack);

/**
 * @brief Peeks at a byte in the stack at a given offset from the top.
 *
 * Peeking non-destructively looks at the value at the specified offset in the
 * stack without removing it.
 *
 * Peeking at offset 0 is equivalent to peeking at the top of the stack.
 *
 * @param stack Pointer to the stack.
 * @param offset The offset from the top of the stack.
 * @return The byte value at the specified offset in the stack.
 */
Byte Stack_peek_offset(T* stack, Byte offset);

/**
 * @brief Peeks at the top value in the stack without removing it.
 * 
 * @param stack Pointer to the stack.
 * @return The byte value at the top of the stack.
 */
Byte Stack_peek(T* stack);


#undef T
#endif // STACK_H