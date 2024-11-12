#include "uxn.h"
#include "debug.h"
#include <stdio.h>

void initUxn(Uxn *uxn) {
  for (int i = 0; i < 0x10000; i++) {
    uxn->ram[i] = 0;
  }
  Stack_init(&uxn->work);
  Stack_init(&uxn->ret);
}

/**
 * Pushes a value onto the working stack of the given Uxn instance.
 *
 * @param uxn Pointer to the Uxn instance.
 * @param value The byte value to be pushed onto the working stack.
 */
void Uxn_push_work(Uxn *uxn, Byte value) {
  Stack_push(&uxn->work, value);
}

/**
 * Pops a value from the working stack of the given Uxn instance.
 *
 * @param uxn Pointer to the Uxn instance.
 * 
 * @return The byte value popped from the working stack.
 */
Byte Uxn_pop_work(Uxn *uxn) {
  return Stack_pop(&uxn->work);
}

/**
 * Peeks at a byte from the working stack at a given offset from the top.
 * 
 * @param uxn Pointer to the Uxn instance.
 * @param offset The offset from the top of the stack. 0 is the top of the stack.
 *
 * @return The byte value at the specified offset in the working stack.
 */
Byte Uxn_peek_work_offset(Uxn *uxn, Byte offset) {
  return Stack_peek_offset(&uxn->work, offset);
}

/**
 * Peeks at the top value in the working stack without removing it.
 * 
 * @param uxn Pointer to the Uxn instance.
 * 
 * @return The byte value at the top of the working stack.
 */
Byte Uxn_peek_work(Uxn *uxn) {
  return Stack_peek(&uxn->work);
}

/**
 * Push a value onto the Return stack
 * 
 * @param uxn Pointer to the Uxn virtual machine instance
 * @param value The byte value to push onto the Return stack
 * 
 * Adds a single byte value to the top of the Return stack. Used primarily
 * for function return addresses and temporary storage during subroutine calls.
 */
void Uxn_push_ret(Uxn *uxn, Byte value) {
  Stack_push(&uxn->ret, value);
}

/**
 * Pops and returns a value from the return stack of the Uxn virtual machine.
 *
 * @param uxn Pointer to the Uxn virtual machine instance
 * 
 * @return The byte value popped from the return stack
 */
Byte Uxn_pop_ret(Uxn *uxn) {
  return Stack_pop(&uxn->ret);
}

/**
 * Peeks at the top value in the return stack without removing it.
 * 
 * @param uxn Pointer to the Uxn virtual machine instance
 * 
 * @return The byte value at the top of the return stack
 */
Byte Uxn_peek_ret(Uxn *uxn) {
  return Stack_peek(&uxn->ret);
}

/**
 * Evaluates the instruction at the given program counter.
 * 
 * @param uxn Pointer to the Uxn virtual machine instance
 * @param pc The program counter to evaluate
 * 
 * @return True if the instruction was successfully evaluated, false otherwise
 */
bool Uxn_eval(Uxn *uxn, Short pc) {
  Byte op = uxn->ram[pc];

  switch (op) {
    case 0x80: // LIT
      Byte literal_value = Uxn_peek_work_offset(uxn, 1);
      Uxn_push_work(uxn, literal_value);
      return true;
    case 0x00: // BRK
      return true;
    case 0x01: // INC
      break;
    // Stack manipulators
    case 0x02: // POP
      break;
    case 0x03: // NIP
      break;
    case 0x04: // SWP
      break;
    case 0x05: // ROT
      break;
    case 0x06: // DUP
      break;
    case 0x07: // OVR
      break;
    // Logical operators
    case 0x08: // EQU
      break;
    case 0x09: // NEQ
      break;
    case 0x0a: // GTH
      break;
    case 0x0b: // LTH
      break;
    // Control flow
    case 0x0c: // JMP
      break;
    case 0x0d: // JCN
      break;
    case 0x0e: // JSR
      break;
    case 0x0f: // STH
      break;
    // Memory operations
    case 0x10: // LDZ
      break;
    case 0x11: // STZ
      break;
    case 0x12: // LDR
      break;
    case 0x13: // STR
      break;
    case 0x14: // LDA
      break;
    case 0x15: // STA
      break;
    case 0x16: // DEI
      break;
    case 0x17: // DEO
      break;
    // Arithmetic operations
    case 0x18: // ADD
      break;
    case 0x19: // SUB
      break;
    case 0x1a: // MUL
      break;
    case 0x1b: // DIV
      break;
    // Bitwise operations
    case 0x1c: // AND
      break;
    case 0x1d: // ORA
      break;
    case 0x1e: // EOR
      break;
    case 0x1f: // SFT
      break;

  }
}