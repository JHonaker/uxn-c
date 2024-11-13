#include "common.h"
#include "stack.h"

#ifndef uxn_h
#define uxn_h

#define T Uxn

typedef struct T T;

// Lifecycle management
void Uxn_init(T* uxn);
void Uxn_destroy(T* uxn);
T *Uxn_new();
void Uxn_delete(T* uxn);

// Stack operations

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

Byte Uxn_read_byte(T* uxn, Short addr);
Short Uxn_read_short(T* uxn, Short addr);

Byte Uxn_read_next_byte(T* uxn);
Short Uxn_read_next_short(T* uxn);

void Uxn_write_byte(T* uxn, Short addr, Byte value);
void Uxn_write_short(T* uxn, Short addr, Short value);


// Interaction with the Uxn instance


/**
 * Evaluates the instruction at the given program counter.
 * 
 * @param uxn Pointer to the Uxn virtual machine instance
 * @param pc The program counter to evaluate
 * 
 * @return True if the instruction was successfully evaluated, false otherwise
 */
bool Uxn_eval(T* uxn, Short pc);

/**
 * Steps the Uxn virtual machine instance.
 * 
 * @param uxn Pointer to the Uxn virtual machine instance
 * 
 * @return True if the Uxn virtual machine instance successfully stepped, false otherwise
 */
bool Uxn_step(T* uxn);

void Uxn_dump(T* uxn);

extern Byte Uxn_dei_dispatch(Byte page);
extern void Uxn_deo_dispatch(Byte page, Byte value);

#undef T
#endif // uxn_h