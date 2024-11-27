#include "uxn.h"
#include "common.h"

extern Byte uxn_dei_dispatch(Uxn *uxn, Byte addr);
extern void uxn_deo_dispatch(Uxn *uxn, Byte addr);

Byte save_stack_ptr(Uxn *uxn, bool return_mode) {
  return return_mode ? uxn_ret_ptr(uxn) : uxn_work_ptr(uxn);
}

void restore_stack_ptr(Uxn *uxn, bool return_mode, Byte ptr) {
  if (return_mode) {
    uxn_set_ret_ptr(uxn, ptr);
  } else {
    uxn_set_work_ptr(uxn, ptr);
  }
}

// Immediate Ops

/**
 * Jump Instant: JMI ( -- )
 *
 * Moves the PC to a relative address at a distance equal to the next short in
 * memory. This opcode has no modes.
 */
Short op_jmi(Uxn *uxn, Short pc) {
  Short rel_addr = uxn_mem_read_short(uxn, pc);
  return pc + (SignedShort)rel_addr + 2;
}

/**
 * Jump Stash Return Instant: JSI ( -- )
 *
 * Pushes PC+2 to the return-stack and moves the PC to a relative address at a
 * distance equal to the next short in memory. This opcode has no modes.
 */
Short op_jsi(Uxn *uxn, Short pc) {
  // TODO: Check if this is correct
  Short to_push = pc + 2;
  uxn_push_short(uxn, to_push, true);
  Short rel_addr = uxn_mem_read_short(uxn, pc);
  return pc + (SignedShort)rel_addr + 2;
}

/**
 * Jump Conditional Instant: JCI (cond8 -- )
 *
 * Pops a byte from the working stack and if it is not zero, moves the PC to a
 * relative address at a distance equal to the next short in memory, otherwise
 * moves PC+2. This opcode has no modes.
 */
Short op_jci(Uxn *uxn, Short pc) {
  Byte cond = uxn_pop_work(uxn);
  if (cond) {
    Short rel_addr = uxn_mem_read_short(uxn, pc);
    return pc + (SignedShort)rel_addr + 2;
  } else {
    return pc + 2;
  }
}

/**
 * Literal: LIT ( -- a)
 *
 * Pushes the next bytes in memory, and moves the PC+2. The LIT opcode always
 * has the keep mode active.
 */
Short op_lit(Uxn *uxn, Short pc, bool return_mode, bool short_mode) {
  Byte literal_value = uxn_mem_read(uxn, pc);
  uxn_push(uxn, literal_value, return_mode);

  if (short_mode) {
    literal_value = uxn_mem_read(uxn, pc + 1);
    uxn_push(uxn, literal_value, return_mode);
    return pc + 2;
  }

  return pc + 1;
}

/**
 * Increment: Inc (a -- a+1)
 *
 * Increments the top value of the stack by one.
 */
Short op_inc(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, a + 1, return_mode);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, a + 1, return_mode);
  }

  return pc;
}

/**
 * Pop: POP (a -- )
 *
 * Removes the value at the top of the stack
 */
Short op_pop(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  if (keep_mode)
    return pc;

  uxn_pop(uxn, return_mode);
  if (short_mode)
    uxn_pop(uxn, return_mode);

  return pc;
}

/**
 * Nip: NIP (a b -- b)
 *
 * Removes the second value from the stack. This is useful to truncate a short
 * into a byte.
 */
Short op_nip(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, b, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, b, return_mode);
  }

  return pc;
}

/**
 * Swap: SWP (a b -- b a)
 *
 * Exchanges the top two values on the stack.
 */
Short op_swp(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, b, return_mode);
    uxn_push_short(uxn, a, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, b, return_mode);
    uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * Rotate: ROT (a b c -- b c a)
 *
 * Rotates three values at the top of the stack, to the left, wrapping around.
 */
Short op_rot(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short c = uxn_pop_short(uxn, return_mode);
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, b, return_mode);
    uxn_push_short(uxn, c, return_mode);
    uxn_push_short(uxn, a, return_mode);
  } else {
    Byte c = uxn_pop(uxn, return_mode);
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, b, return_mode);
    uxn_push(uxn, c, return_mode);
    uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * Duplicate: DUP (a -- a a)
 *
 * Duplicates the value at the top of the stack.
 */
Short op_dup(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, a, return_mode);
    uxn_push_short(uxn, a, return_mode);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, a, return_mode);
    uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * Over: OVR (a b -- a b a)
 *
 * Duplicates the second value at the top of the stack.
 */
Short op_ovr(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, a, return_mode);
    uxn_push_short(uxn, b, return_mode);
    uxn_push_short(uxn, a, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, a, return_mode);
    uxn_push(uxn, b, return_mode);
    uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * Equal: EQU (a b -- bool8)
 *
 * Pushes 01 to the stack if the two values at the top of the stack are equal,
 * 00 otherwise.
 */
Short op_equ(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  bool result = false;

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    result = (a == b);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    result = (a == b);
  }
  if (keep_mode)
    restore_stack_ptr(uxn, return_mode, ptr);

  uxn_push(uxn, result ? 0x01 : 0x00, return_mode);

  return pc;
}

/**
 * Not Equal: NEQ (a b -- bool8)
 *
 * Pushes 01 to the stack if the two values at the top of the stack are not
 * equal, 00 otherwise.
 */
Short op_neq(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  bool result = false;

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    result = (a != b);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    result = (a != b);
  }
  if (keep_mode)
    restore_stack_ptr(uxn, return_mode, ptr);

  uxn_push(uxn, result ? 0x01 : 0x00, return_mode);

  return pc;
}

/**
 * Greater Than: GTH (a b -- bool8)
 *
 * Pushes 01 to the stack if a > b, 00 otherwise.
 */
Short op_gth(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  bool result = false;

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    result = (a > b);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    result = (a > b);
  }

  if (keep_mode)
    restore_stack_ptr(uxn, return_mode, ptr);

  uxn_push(uxn, result ? 0x01 : 0x00, return_mode);

  return pc;
}

/**
 * Less Than: LTH (a b -- bool8)
 *
 * Pushes 01 to the stack if a < b, 00 otherwise.
 */
Short op_lth(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  bool result = false;

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    result = (a < b);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    result = (a < b);
  }
  if (keep_mode)
    restore_stack_ptr(uxn, return_mode, ptr);

  uxn_push(uxn, result ? 0x01 : 0x00, return_mode);

  return pc;
}

/**
 * Jump: JMP (addr -- )
 *
 * Move the PC by a relative distance equal to the signed byte on the top of the
 * stack, or to an absolute address in short mode.
 */
Short op_jmp(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    // Jump to absolute address
    Short addr = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    return addr;
  } else {
    // Jump by a relative address
    SignedByte rel_addr = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    return pc + rel_addr;
  }
}

/**
 * Jump Conditional: JCN (cond8 addr -- )
 *
 * If the byte preceeding the address is not 00, moves the PC by a signed value
 * equal to the byte on the top of the stack, or to an absolute address in short
 * mode.
 */
Short op_jcn(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short addr = uxn_pop_short(uxn, return_mode);

    Byte test_byte = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    // In short mode, jump to an absolute address
    return test_byte != 0x00 ? addr : pc;
  } else {
    SignedByte rel_addr = uxn_pop(uxn, return_mode);
    Byte test_byte = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    // Otherwise jump by a relative address
    return test_byte != 0x00 ? pc + rel_addr : pc;
  }
}

/**
 * Jump Stack Return: JSR (addr -- | ret*)
 *
 * Pushes the PC to the return stack and moves the PC by a signed value equal to
 * the byte on the top of the stack, or to an absolute address in short mode.
 *
 * Using return mode swaps the operation on the working and return stacks.
 */
Short op_jsr(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  uxn_push_short(uxn, pc, !return_mode);

  if (short_mode) {
    Short addr = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    return addr;
  } else {
    SignedByte rel_addr = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    return pc + rel_addr;
  }
}

/**
 * Stash: STH (a -- | a)
 *
 * Moves the value at the top of the stack to the return stack.
 *
 * With return mode active, the stacks are exchanged, and the value is moved
 * from the return stack to the working stack.
 */
Short op_sth(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, a, !return_mode);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, a, !return_mode);
  }

  return pc;
}

/**
 * Load Zero Page: LDZ (addr8 -- a)
 *
 * Pushes the value at an address within the first 256 bytes of memeory, to the
 * top of the stack.
 */
Short op_ldz(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  Byte addr = uxn_pop(uxn, return_mode);

  if (keep_mode)
    restore_stack_ptr(uxn, return_mode, ptr);

  if (short_mode) {
    Short a = uxn_zero_page_read_short(uxn, addr);

    uxn_push_short(uxn, a, return_mode);
  } else {
    Byte a = uxn_mem_read(uxn, addr);
    uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * Store Zero Page: STZ (a addr8 -- )
 *
 * Writes a value to an address within the first 256 bytes of memory.
 */
Short op_stz(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  Byte addr = uxn_pop(uxn, return_mode);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_zero_page_write_short(uxn, addr, a);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_zero_page_write(uxn, addr, a);
  }

  return pc;
}

/**
 * Load Relative: LDR (addr8 -- a)
 *
 * Pushes a value at a relative address in relation to the PC, within a range
 * between -128 and +127 bytes, to the top of the stack.
 */
Short op_ldr(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  SignedByte rel_addr = uxn_pop(uxn, return_mode);

  if (keep_mode)
    restore_stack_ptr(uxn, return_mode, ptr);

  Short load_addr = pc + rel_addr;

  if (short_mode) {
    Short a = uxn_mem_read_short(uxn, load_addr);
    uxn_push_short(uxn, a, return_mode);
  } else {
    Byte a = uxn_mem_read(uxn, load_addr);
    uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * Store Relative: STR (a addr8 -- )
 *
 * Writes a value to a relative address in relation to the PC, within a range
 * between -128 and +127 bytes.
 */
Short op_str(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  SignedByte rel_addr = uxn_pop(uxn, return_mode);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    Short store_addr = pc + rel_addr;
    uxn_mem_write_short(uxn, store_addr, a);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    Short store_addr = pc + rel_addr;

    uxn_mem_write(uxn, store_addr, a);
  }

  return pc;
}

/**
 * Load Absolute: LDA (addr16 -- a)
 *
 * Pushes a value at an absolute address in memory to the top of the stack.
 */
Short op_lda(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  Short addr = uxn_pop_short(uxn, return_mode);

  if (keep_mode)
    restore_stack_ptr(uxn, return_mode, ptr);

  if (short_mode) {
    Short a = uxn_mem_read_short(uxn, addr);
    uxn_push_short(uxn, a, return_mode);
  } else {
    Byte a = uxn_mem_read(uxn, addr);
    uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * Store Absolute: STA (a addr16 -- )
 *
 * Writes a value to an absolute address in memory.
 */
Short op_sta(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  Short addr = uxn_pop_short(uxn, return_mode);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);
    
    uxn_mem_write_short(uxn, addr, a);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_mem_write(uxn, addr, a);
  }

  return pc;
}

/**
 * Device Input: DEI (device8 -- a)
 *
 * Pushes a value from the device page to the top of the stack.
 * The target device might capture the reading to trigger an I/O event.
 */
Short op_dei(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  Byte addr = uxn_pop(uxn, return_mode);

  if (keep_mode)
    restore_stack_ptr(uxn, return_mode, ptr);

  if (short_mode) {
    Byte high_a = uxn_dei_dispatch(uxn, addr);
    Byte low_a = uxn_dei_dispatch(uxn, addr + 1);

    uxn_push(uxn, high_a, return_mode);
    uxn_push(uxn, low_a, return_mode);
  } else {
    Byte a = uxn_dei_dispatch(uxn, addr);

    uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * Device Output: DEO (a device8 -- )
 *
 * Writes a value to the device page. The target device might capture the
 * writing to trigger an I/O event.
 */
Short op_deo(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  Byte addr = uxn_pop(uxn, return_mode);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_dev_write_short(uxn, addr, a);

    uxn_deo_dispatch(uxn, addr);
    uxn_deo_dispatch(uxn, addr + 1);

  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_dev_write(uxn, addr, a);

    uxn_deo_dispatch(uxn, addr);
  }

  return pc;
}

/**
 * Add: ADD (a b -- a+b)
 *
 * Pushes the sum of the two values at the top of the stack.
 */
Short op_add(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    Short sum = a + b;

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, sum, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, a + b, return_mode);
  }

  return pc;
}

/**
 * Subtract: SUB (a b -- a-b)
 *
 * Pushes the difference of the first value minus the second, to the top of the
 * stack.
 */
Short op_sub(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    Short diff = a - b;

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, diff, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, a - b, return_mode);
  }

  return pc;
}

/**
 * Multiply: MUL (a b -- a*b)
 *
 * Pushes the product of the first and second values at the top of the stack.
 */
Short op_mul(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    Short prod = a * b;

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, prod, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, a * b, return_mode);
  }

  return pc;
}

/**
 * Divide: DIV (a b -- a/b)
 *
 * Pushes the quotient of the first value over the second, to the top of the
 * stack. Division by zero pushes zero to the stack. The rounding direction is
 * toward zero.
 */
Short op_div(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    if (b == 0) {
      uxn_push_short(uxn, 0, return_mode);
    } else {
      Short quot = a / b;
      uxn_push_short(uxn, quot, return_mode);
    }
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    if (b == 0) {
      uxn_push(uxn, 0, return_mode);
    } else {
      uxn_push(uxn, a / b, return_mode);
    }
  }

  return pc;
}

/**
 * And: AND (a b -- a&b)
 *
 * Pushes the bitwise AND of the two values at the top of the stack.
 */
Short op_and(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, a & b, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, a & b, return_mode);
  }

  return pc;
}

/**
 * Or: OR (a b -- a|b)
 *
 * Pushes the bitwise OR of the two values at the top of the stack.
 */
Short op_ora(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, a | b, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, a | b, return_mode);
  }

  return pc;
}

/**
 * Exclusive Or: EOR (a b -- a^b)
 *
 * Pushes the bitwise XOR of the two values at the top of the stack.
 */
Short op_eor(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, a ^ b, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push(uxn, a ^ b, return_mode);
  }

  return pc;
}

/**
 * Shift: SFT (a shift8 -- c)
 *
 * Shifts the bits of the second value of the stack to the left or right,
 * depending on the control value at the top of the stack. The high nibble of
 * the control value indicates how many bits to shift left, the low nibble how
 * many bits to shift right. The rightward shift is done first.
 */
Short op_sft(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Byte ptr = save_stack_ptr(uxn, return_mode);

  Byte shift = uxn_pop(uxn, return_mode);

  Byte n_left = high_nibble(shift);
  Byte n_right = low_nibble(shift);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    Short result = (a >> n_right) << n_left;

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    uxn_push_short(uxn, result, return_mode);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      restore_stack_ptr(uxn, return_mode, ptr);

    Byte result = (a >> n_right) << n_left;

    uxn_push(uxn, result, return_mode);
  }

  return pc;
}