#include "common.h"
#include "stack.h"

#ifndef uxn_h
#define uxn_h

#define T Uxn

typedef struct T T;

// Lifecycle management
void Uxn_init(T* uxn);
void Uxn_destroy(T* uxn);
T* Uxn_new();
void Uxn_delete(T* uxn);

// Stack operations

void Uxn_stack_zero(T* uxn);
Byte Uxn_work_ptr(T* uxn);
Byte Uxn_ret_ptr(T* uxn);
void Uxn_set_work_ptr(T* uxn, Byte ptr);
void Uxn_set_ret_ptr(T* uxn, Byte ptr);

/**
 * Pushes a value onto the working stack of the given Uxn instance.
 *
 * @param uxn Pointer to the Uxn instance.
 * @param value The byte value to be pushed onto the working stack.
 */
void Uxn_push_work(T* uxn, Byte value);

/**
 * Pops a value from the working stack of the given Uxn instance.
 *
 * @param uxn Pointer to the Uxn instance.
 * 
 * @return The byte value popped from the working stack.
 */
Byte Uxn_pop_work(T* uxn);

/**
 * Peeks at a byte from the working stack at a given offset from the top.
 * 
 * @param uxn Pointer to the Uxn instance.
 * @param offset The offset from the top of the stack. 0 is the top of the stack.
 *
 * @return The byte value at the specified offset in the working stack.
 */
Byte Uxn_peek_work_offset(T* uxn, Byte offset);

/**
 * Peeks at the top value in the working stack without removing it.
 * 
 * @param uxn Pointer to the Uxn instance.
 * 
 * @return The byte value at the top of the working stack.
 */
Byte Uxn_peek_work(T* uxn);

/**
 * Push a value onto the Return stack
 * 
 * @param uxn Pointer to the Uxn virtual machine instance
 * @param value The byte value to push onto the Return stack
 */
void Uxn_push_ret(T* uxn, Byte value);

/**
 * Pops and returns a value from the return stack of the Uxn virtual machine.
 *
 * @param uxn Pointer to the Uxn virtual machine instance
 * 
 * @return The byte value popped from the return stack
 */
Byte Uxn_pop_ret(T* uxn);

/**
 * Peeks at the top value in the return stack without removing it.
 * 
 * @param uxn Pointer to the Uxn virtual machine instance
 * 
 * @return The byte value at the top of the return stack
 */
Byte Uxn_peek_ret(T* uxn);

// Memory operations

void Uxn_mem_zero(T* uxn, bool include_zero_page);

void Uxn_mem_load(T* uxn, Byte* program, Short size, Short addr);
Byte Uxn_mem_read(T* uxn, Short addr);
Short Uxn_mem_read_short(T* uxn, Short addr);

void Uxn_mem_write(T* uxn, Short addr, Byte value);

// Interaction with the Uxn instance

Byte Uxn_dev_read(T* uxn, Byte addr);
void Uxn_dev_write(T* uxn, Byte addr, Byte value);
void Uxn_dev_zero(Uxn* uxn);

/**
 * Evaluates the instruction at the given program counter.
 * 
 * @param uxn Pointer to the Uxn virtual machine instance
 * @param pc The program counter to evaluate
 * 
 * @return True if the instruction was successfully evaluated, false otherwise
 */
bool Uxn_eval(T* uxn, Short pc);

void Uxn_dump(T* uxn);

#undef T
#endif // uxn_h