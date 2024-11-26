#include "common.h"
#include "stack.h"

#ifndef uxn_h
#define uxn_h

#define T Uxn

typedef struct T T;

// Lifecycle management
void uxn_init(T *uxn, void *screen);
void uxn_destroy(T *uxn);
T *uxn_new(void *screen);
void uxn_delete(T *uxn);

// Stack operations

void uxn_stack_zero(T *uxn);
Byte uxn_work_ptr(T *uxn);
Byte uxn_ret_ptr(T *uxn);
void uxn_set_work_ptr(T *uxn, Byte ptr);
void uxn_set_ret_ptr(T *uxn, Byte ptr);

/**
 * Pushes a value onto the working stack of the given Uxn instance.
 *
 * @param uxn Pointer to the Uxn instance.
 * @param value The byte value to be pushed onto the working stack.
 */
void uxn_push_work(T *uxn, Byte value);

/**
 * Pops a value from the working stack of the given Uxn instance.
 *
 * @param uxn Pointer to the Uxn instance.
 *
 * @return The byte value popped from the working stack.
 */
Byte uxn_pop_work(T *uxn);

/**
 * Peeks at a byte from the working stack at a given offset from the top.
 *
 * @param uxn Pointer to the Uxn instance.
 * @param offset The offset from the top of the stack. 0 is the top of the
 * stack.
 *
 * @return The byte value at the specified offset in the working stack.
 */
Byte uxn_peek_work_offset(T *uxn, Byte offset);

/**
 * Peeks at the top value in the working stack without removing it.
 *
 * @param uxn Pointer to the Uxn instance.
 *
 * @return The byte value at the top of the working stack.
 */
Byte uxn_peek_work(T *uxn);

/**
 * Push a value onto the Return stack
 *
 * @param uxn Pointer to the Uxn virtual machine instance
 * @param value The byte value to push onto the Return stack
 */
void uxn_push_ret(T *uxn, Byte value);

/**
 * Pops and returns a value from the return stack of the Uxn virtual machine.
 *
 * @param uxn Pointer to the Uxn virtual machine instance
 *
 * @return The byte value popped from the return stack
 */
Byte uxn_pop_ret(T *uxn);

/**
 * Peeks at the top value in the return stack without removing it.
 *
 * @param uxn Pointer to the Uxn virtual machine instance
 *
 * @return The byte value at the top of the return stack
 */
Byte uxn_peek_ret(T *uxn);

// Memory operations

void uxn_mem_zero(T *uxn, bool include_zero_page);

void uxn_mem_load(T *uxn, Byte *program, unsigned long size, size_t addr);
Byte uxn_mem_read(T *uxn, size_t addr);
Short uxn_mem_read_short(T *uxn, size_t addr);

void uxn_mem_buffer_read(Uxn *uxn, Short size, Byte buffer[size],  size_t addr);

void uxn_mem_write(T *uxn, size_t addr, Byte value);

Byte uxn_page_read(T *uxn, Short page, size_t addr);
void uxn_page_write(T *uxn, Short page, size_t addr, Byte value);
void uxn_page_load(Uxn *uxn, Byte program[], unsigned long size, size_t page, size_t addr);

// Interaction with the Uxn instance

Byte uxn_dev_read(T *uxn, Byte addr);
Short uxn_dev_read_short(T *uxn, Byte addr);
void uxn_dev_write(T *uxn, Byte addr, Byte value);
void uxn_dev_write_short(T *uxn, Byte addr, Short value);
void uxn_dev_zero(Uxn *uxn);

void *uxn_get_screen(T *uxn);

void *uxn_get_open_files(T *uxn);
void uxn_set_open_files(T *uxn, void *files);

/**
 * Evaluates the instruction at the given program counter.
 *
 * @param uxn Pointer to the Uxn virtual machine instance
 * @param pc The program counter to evaluate
 *
 * @return True if the instruction was successfully evaluated, false otherwise
 */
bool uxn_eval(T *uxn, Short pc);

void uxn_dump(T *uxn);

void uxn_dump_page(T *uxn, Short page);

#undef T
#endif // uxn_h