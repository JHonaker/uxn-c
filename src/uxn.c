#include "uxn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Byte Uxn_dei_dispatch(Uxn* uxn, Byte addr);
extern void Uxn_deo_dispatch(Uxn* uxn, Byte addr);

int high_nibble(Byte byte) {
  return (byte & 0xf0) >> 4;
}

int low_nibble(Byte byte) {
  return byte & 0x0f;
}

bool is_keep_mode(Byte op) {
  return (op & 0x80) == 0x80;
}

bool is_return_mode(Byte op) {
  return (op & 0x40) == 0x40;
}

bool is_short_mode(Byte op) {
  return (op & 0x20) == 0x20;
}

Byte opcode(Byte op) {
  return op & 0x1f;
}

struct Uxn {
  Byte ram[RAM_PAGE_SIZE * RAM_PAGES];
  Byte dev[DEV_PAGE_SIZE];
  Stack* work;
  Stack* ret;
};

void Uxn_init(Uxn* uxn) {
  if (uxn) {
    *uxn = (Uxn) {
      .ram = { 0 },
      .dev = { 0 },
      .work = Stack_new(),
      .ret = Stack_new(),
    };
  }
}

void Uxn_destroy(Uxn* uxn) {
  if (uxn) {
    for (int i = 0; i < RAM_PAGE_SIZE * RAM_PAGES; i++) {
      uxn->ram[i] = 0;
    }
    for (int i = 0; i < DEV_PAGE_SIZE; i++) {
      uxn->dev[i] = 0;
    }
    Stack_delete(uxn->work);
    Stack_delete(uxn->ret);
  }
}

Uxn* Uxn_new() {
  Uxn* uxn = malloc(sizeof(Uxn));
  Uxn_init(uxn);
  return uxn;
}

void Uxn_delete(Uxn* uxn) {
  if (uxn) {
    Uxn_destroy(uxn);
    free(uxn);
  }
}

// Stack operations

void Uxn_stack_zero(Uxn* uxn) {
  Stack_set_ptr(uxn->work, 0);
  Stack_set_ptr(uxn->ret, 0);
}

Byte Uxn_work_ptr(Uxn* uxn) {
  return Stack_get_ptr(uxn->work);
}

Byte Uxn_ret_ptr(Uxn* uxn) {
  return Stack_get_ptr(uxn->ret);
}

void Uxn_set_work_ptr(Uxn* uxn, Byte ptr) {
  Stack_set_ptr(uxn->work, ptr);
}

void Uxn_set_ret_ptr(Uxn* uxn, Byte ptr) {
  Stack_set_ptr(uxn->ret, ptr);
}

void Uxn_push_work(Uxn* uxn, Byte value) {
  Stack_push(uxn->work, value);
}

Byte Uxn_pop_work(Uxn* uxn) {
  return Stack_pop(uxn->work);
}

Byte Uxn_peek_work_offset(Uxn* uxn, Byte offset) {
  return Stack_peek_offset(uxn->work, offset);
}

Byte Uxn_peek_work(Uxn* uxn) {
  return Stack_peek(uxn->work);
}

void Uxn_push_ret(Uxn* uxn, Byte value) {
  Stack_push(uxn->ret, value);
}

Byte Uxn_pop_ret(Uxn* uxn) {
  return Stack_pop(uxn->ret);
}

Byte Uxn_peek_ret(Uxn* uxn) {
  return Stack_peek(uxn->ret);
}

void Uxn_push(Uxn* uxn, Byte value, bool to_return_stack) {
  if (to_return_stack) {
    Uxn_push_ret(uxn, value);
  } else {
    Uxn_push_work(uxn, value);
  }
}

Byte Uxn_pop(Uxn* uxn, bool from_return_stack) {
  if (from_return_stack) {
    return Uxn_pop_ret(uxn);
  } else {
    return Uxn_pop_work(uxn);
  }
}

// Memory operations

void Uxn_mem_zero(Uxn* uxn, bool soft) {
  for (int i = (soft ? RESET_VECTOR : 0); i < RAM_PAGE_SIZE * RAM_PAGES; i++) {
    uxn->ram[i] = 0;
  }
}

void Uxn_mem_load(Uxn* uxn, Byte* program, Short size, Short addr) {
  memcpy(&uxn->ram[addr], program, size);
}

Byte Uxn_mem_read(Uxn* uxn, Short address) {
  return uxn->ram[address];
}

Short Uxn_mem_read_short(Uxn* uxn, Short address) {
  return (uxn->ram[address] << 8) | uxn->ram[address + 1];
}

void Uxn_mem_write(Uxn* uxn, Short address, Byte value) {
  uxn->ram[address] = value;
}

// Device operations

void Uxn_dev_zero(Uxn* uxn) {
  for (int i = 0; i < DEV_PAGE_SIZE; i++) {
    uxn->dev[i] = 0;
  }
}

Byte Uxn_dev_read(Uxn* uxn, Byte addr) {
  return uxn->dev[addr];
}

void Uxn_dev_write(Uxn* uxn, Byte addr, Byte value) {
  uxn->dev[addr] = value;
}

// Immediate Ops

/**
 * @brief Jump Instant: JMI ( -- )
 * 
 * Moves the PC to a relative address at a distance equal to the next short in
 * memory. This opcode has no modes.
 */
Short op_jmi(Uxn* uxn, Short pc) {
  Short rel_addr = Uxn_mem_read_short(uxn, pc);
  return pc + (SignedShort) rel_addr;
}

/**
 * @brief Jump Stash Return Instant: JSI ( -- )
 * 
 * Pushes PC+2 to the return-stack and moves the PC to a relative address at a
 * distance equal to the next short in memory. This opcode has no modes.
 */
Short op_jsi(Uxn* uxn, Short pc) {
  // TODO: Check if this is correct
  Short to_push = pc + 2;
  Uxn_push_ret(uxn, to_push >> 8);
  Uxn_push_ret(uxn, to_push & 0xff);
  Short rel_addr = Uxn_mem_read_short(uxn, pc);
  return pc + (SignedShort) rel_addr;
}

/**
 * @brief Jump Conditional Instant: JCI (cond8 -- )
 *
 * Pops a byte from the working stack and if it is not zero, moves the PC to a
 * relative address at a distance equal to the next short in memory, otherwise
 * moves PC+2. This opcode has no modes.
 */
Short op_jci(Uxn* uxn, Short pc) {
  Byte cond = Uxn_pop_work(uxn);
  if (cond) {
    Short rel_addr = Uxn_mem_read_short(uxn, pc);
    return pc + (SignedShort) rel_addr;
  } else {
    return pc + 2;
  }
}

/**
 * @brief Literal: LIT ( -- a)
 *
 * Pushes the next bytes in memory, and moves the PC+2. The LIT opcode always
 * has the keep mode active.
 */
Short op_lit(Uxn* uxn, Short pc, bool return_mode, bool short_mode) {
  Byte literal_value = Uxn_mem_read(uxn, pc);
  Uxn_push(uxn, literal_value, return_mode);

  if (short_mode) {
    literal_value = Uxn_mem_read(uxn, pc + 1);
    Uxn_push(uxn, literal_value, return_mode);
    return pc + 2;
  }

  return pc + 1;
}

/**
 * @brief Increment: Inc (a -- a+1)
 * 
 * Increments the top value of the stack by one.
 */
Short op_inc(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_byte = Uxn_pop(uxn, return_mode);
    Byte high_byte = Uxn_pop(uxn, return_mode);
    
    if (keep_mode) Stack_set_ptr(stack, ptr);
    
    Short a = (high_byte << 8) | low_byte;
    a += 1;
    Uxn_push(uxn, a >> 8, return_mode);
    Uxn_push(uxn, a & 0xff, return_mode);
  } else {
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);
    
    Uxn_push(uxn, a + 1, return_mode);
  }

  return pc;
}

/**
 * @brief Pop: POP (a -- )
 * 
 * Removes the value at the top of the stack
 */
Short op_pop(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  if (keep_mode) return pc;

  Uxn_pop(uxn, return_mode);
  if (short_mode) Uxn_pop(uxn, return_mode);

  return pc;
}

/**
 * @brief Nip: NIP (a b -- b)
 *
 * Removes the second value from the stack. This is useful to truncate a short
 * into a byte.
 */
Short op_nip(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Byte b = Uxn_pop(uxn, return_mode);
  Byte a = Uxn_pop(uxn, return_mode);

  if (short_mode) {
    Uxn_pop(uxn, return_mode);
    Uxn_pop(uxn, return_mode);    
  }

  if (keep_mode) Stack_set_ptr(stack, ptr);
  
  if (short_mode) {
    Uxn_push(uxn, a, return_mode);
    Uxn_push(uxn, b, return_mode);
  } else {
    Uxn_push(uxn, b, return_mode);
  }

  return pc;
}

/**
 * @brief Swap: SWP (a b -- b a)
 *
 * Exchanges the top two values on the stack.
 */
Short op_swp(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, high_b, return_mode);
    Uxn_push(uxn, low_b, return_mode);
    Uxn_push(uxn, high_a, return_mode);
    Uxn_push(uxn, low_a, return_mode);
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, b, return_mode);
    Uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * @brief Rotate: ROT (a b c -- b c a)
 * 
 * Rotates three values at the top of the stack, to the left, wrapping around.
 */
Short op_rot(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_c = Uxn_pop(uxn, return_mode);
    Byte high_c = Uxn_pop(uxn, return_mode);
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, high_b, return_mode);
    Uxn_push(uxn, low_b, return_mode);
    Uxn_push(uxn, high_c, return_mode);
    Uxn_push(uxn, low_c, return_mode);
    Uxn_push(uxn, high_a, return_mode);
    Uxn_push(uxn, low_a, return_mode);
  } else {
    Byte c = Uxn_pop(uxn, return_mode);
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, b, return_mode);
    Uxn_push(uxn, c, return_mode);
    Uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * @brief Duplicate: DUP (a -- a a)
 * 
 * Duplicates the value at the top of the stack.
 */
Short op_dup(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, high_a, return_mode);
    Uxn_push(uxn, low_a, return_mode);
    Uxn_push(uxn, high_a, return_mode);
    Uxn_push(uxn, low_a, return_mode);
  } else {
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, a, return_mode);
    Uxn_push(uxn, a, return_mode);
  }

  return pc;
}


/**
 * @brief Over: OVR (a b -- a b a)
 * 
 * Duplicates the second value at the top of the stack.
 */
Short op_ovr(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, high_a, return_mode);
    Uxn_push(uxn, low_a, return_mode);
    Uxn_push(uxn, high_b, return_mode);
    Uxn_push(uxn, low_b, return_mode);
    Uxn_push(uxn, high_a, return_mode);
    Uxn_push(uxn, low_a, return_mode);
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, a, return_mode);
    Uxn_push(uxn, b, return_mode);
    Uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * @brief Equal: EQU (a b -- bool8)
 *
 * Pushes 01 to the stack if the two values at the top of the stack are equal,
 * 00 otherwise.
 */
Short op_equ(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  bool result = false;

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    result = (high_a == high_b && low_a == low_b);
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    result = (a == b);
  }
  if (keep_mode) Stack_set_ptr(stack, ptr);

  Uxn_push(uxn, result ? 0x01 : 0x00, return_mode);

  return pc;
}

/**
 * @brief Not Equal: NEQ (a b -- bool8)
 *
 * Pushes 01 to the stack if the two values at the top of the stack are not
 * equal, 00 otherwise.
 */
Short op_neq(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  bool result = false;

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    result = (high_a != high_b || low_a != low_b);
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    result = (a != b);
  }
  if (keep_mode) Stack_set_ptr(stack, ptr);

  Uxn_push(uxn, result ? 0x01 : 0x00, return_mode);

  return pc;
}

/**
 * @brief Greater Than: GTH (a b -- bool8)
 *
 * Pushes 01 to the stack if a > b, 00 otherwise.
 */
Short op_gth(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  bool result = false;

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    Short a = (high_a << 8) | low_a;
    Short b = (high_b << 8) | low_b;

    result = (a > b);
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    result = (a > b);
  }
  if (keep_mode) Stack_set_ptr(stack, ptr);

  Uxn_push(uxn, result ? 0x01 : 0x00, return_mode);

  return pc;
}

/**
 * @brief Less Than: LTH (a b -- bool8)
 *
 * Pushes 01 to the stack if a < b, 00 otherwise.
 */
Short op_lth(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  bool result = false;

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    Short a = (high_a << 8) | low_a;
    Short b = (high_b << 8) | low_b;

    result = (a < b);
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    result = (a < b);
  }
  if (keep_mode) Stack_set_ptr(stack, ptr);

  Uxn_push(uxn, result ? 0x01 : 0x00, return_mode);

  return pc;
}

/**
 * @brief Jump: JMP (addr -- )
 *
 * Move the PC by a relative distance equal to the signed byte on the top of the
 * stack, or to an absolute address in short mode.
 */
Short op_jmp(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    // Jump to absolute address
    Byte low_addr = Uxn_pop(uxn, return_mode);
    Byte high_addr = Uxn_pop(uxn, return_mode);
    Short addr = (high_addr << 8) | low_addr;

    if (keep_mode) Stack_set_ptr(stack, ptr);

    return addr;
  } else {
    // Jump by a relative address
    SignedByte rel_addr = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    return pc + rel_addr;
  }
}

/**
 * @brief Jump Conditional: JCN (cond8 addr -- )
 *
 * If the byte preceeding the address is not 00, moves the PC by a signed value
 * equal to the byte on the top of the stack, or to an absolute address in short
 * mode.
 */
Short op_jcn(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_addr = Uxn_pop(uxn, return_mode);
    Byte high_addr = Uxn_pop(uxn, return_mode);
    Short addr = (high_addr << 8) | low_addr;

    Byte test_byte = Uxn_pop(uxn, return_mode);
    
    if (keep_mode) Stack_set_ptr(stack, ptr);

    // In short mode, jump to an absolute address
    return test_byte != 0x00 ? addr : pc;
  } else {
    SignedByte rel_addr = Uxn_pop(uxn, return_mode);
    Byte test_byte = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    // Otherwise jump by a relative address
    return test_byte != 0x00 ? pc + rel_addr : pc;
  }
}

/**
 * @brief Jump Stack Return: JSR (addr -- | ret*) 
 *
 * Pushes the PC to the return stack and moves the PC by a signed value equal to
 * the byte on the top of the stack, or to an absolute address in short mode.
 * 
 * Using return mode swaps the operation on the working and return stacks.
 */
Short op_jsr(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);
  
  Uxn_push(uxn, pc, !return_mode);

  if (short_mode) {
    Byte low_addr = Uxn_pop(uxn, return_mode);
    Byte high_addr = Uxn_pop(uxn, return_mode);
    Short addr = (high_addr << 8) | low_addr;

    if (keep_mode) Stack_set_ptr(stack, ptr);

    return addr;
  } else {
    SignedByte rel_addr = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    return pc + rel_addr;
  }
}

/**
 * @brief Stash: STH (a -- | a)
 *
 * Moves the value at the top of the stack to the return stack.
 *
 * With return mode active, the stacks are exchanged, and the value is moved
 * from the return stack to the working stack.
 */
Short op_sth(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, high_a, !return_mode);
    Uxn_push(uxn, low_a, !return_mode);
  } else {
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, a, !return_mode);
  }

  return pc;
}

/**
 * @brief Load Zero Page: LDZ (addr8 -- a)
 *
 * Pushes the value at an address within the first 256 bytes of memeory, to the
 * top of the stack.
 */
Short op_ldz(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Byte addr = Uxn_pop(uxn, return_mode);

  if (keep_mode) Stack_set_ptr(stack, ptr);

  if (short_mode) {
    Byte high_a = Uxn_mem_read(uxn, addr);
    Byte low_a = Uxn_mem_read(uxn, addr + 1);

    Uxn_push(uxn, high_a, return_mode);
    Uxn_push(uxn, low_a, return_mode);
  } else {
    Byte a = Uxn_mem_read(uxn, addr);
    Uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * @brief Store Zero Page: STZ (a addr8 -- )
 * 
 * Writes a value to an address within the first 256 bytes of memory.
 */
Short op_stz(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Byte addr = Uxn_pop(uxn, return_mode);

  if (short_mode) {
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_mem_write(uxn, addr, high_a);
    Uxn_mem_write(uxn, addr + 1, low_a);
  } else {
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_mem_write(uxn, addr, a);
  }

  return pc;
}

/** 
 * @brief Load Relative: LDR (addr8 -- a)
 *
 * Pushes a value at a relative address in relation to the PC, within a range
 * between -128 and +127 bytes, to the top of the stack.
 */
Short op_ldr(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  SignedByte rel_addr = Uxn_pop(uxn, return_mode);

  if (keep_mode) Stack_set_ptr(stack, ptr);

  Short load_addr = pc + rel_addr;

  if (short_mode) {
    Byte high_a = Uxn_mem_read(uxn, load_addr);
    Byte low_a = Uxn_mem_read(uxn, load_addr + 1);

    Uxn_push(uxn, high_a, return_mode);
    Uxn_push(uxn, low_a, return_mode);
  } else {
    Byte a = Uxn_mem_read(uxn, load_addr);
    Uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/** 
 * @brief Store Relative: STR (a addr8 -- )
 *
 * Writes a value to a relative address in relation to the PC, within a range
 * between -128 and +127 bytes.
 */
Short op_str(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  SignedByte rel_addr = Uxn_pop(uxn, return_mode);

  if (short_mode) {
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Short store_addr = pc + rel_addr;

    Uxn_mem_write(uxn, store_addr, high_a);
    Uxn_mem_write(uxn, store_addr + 1, low_a);
  } else {
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Short store_addr = pc + rel_addr;

    Uxn_mem_write(uxn, store_addr, a);
  }

  return pc;
}

/** 
 * @brief Load Absolute: LDA (addr16 -- a)
 *
 * Pushes a value at an absolute address in memory to the top of the stack.
 */
Short op_lda(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Short addr = Uxn_pop(uxn, return_mode);

  if (keep_mode) Stack_set_ptr(stack, ptr);

  if (short_mode) {
    Byte high_a = Uxn_mem_read(uxn, addr);
    Byte low_a = Uxn_mem_read(uxn, addr + 1);

    Uxn_push(uxn, high_a, return_mode);
    Uxn_push(uxn, low_a, return_mode);
  } else {
    Byte a = Uxn_mem_read(uxn, addr);
    Uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * @brief Store Absolute: STA (a addr16 -- )
 * 
 * Writes a value to an absolute address in memory.
 */
Short op_sta(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Short addr = Uxn_pop(uxn, return_mode);

  if (short_mode) {
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_mem_write(uxn, addr, high_a);
    Uxn_mem_write(uxn, addr + 1, low_a);
  } else {
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_mem_write(uxn, addr, a);
  }

  return pc;
}

/**
 * @brief Device Input: DEI (device8 -- a)
 * 
 * Pushes a value from the device page to the top of the stack.
 * The target device might capture the reading to trigger an I/O event.
 */
Short op_dei(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);
  
  Byte addr = Uxn_pop(uxn, return_mode);

  if (keep_mode) Stack_set_ptr(stack, ptr);

  if (short_mode) {
    Byte low_a = Uxn_dev_read(uxn, addr);
    Byte high_a = Uxn_dev_read(uxn, addr + 1);

    Uxn_dei_dispatch(uxn, addr);
    Uxn_dei_dispatch(uxn, addr + 1);

    Uxn_push(uxn, high_a, return_mode);
    Uxn_push(uxn, low_a, return_mode);
  } else {
    Byte a = Uxn_dev_read(uxn, addr);

    Uxn_dei_dispatch(uxn, addr);

    Uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * @brief Device Output: DEO (a device8 -- )
 *
 * Writes a value to the device page. The target device might capture the
 * writing to trigger an I/O event.
 */
Short op_deo(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);
  
  Byte addr = Uxn_pop(uxn, return_mode);

  if (short_mode) {
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_dev_write(uxn, addr, high_a);
    Uxn_dev_write(uxn, addr + 1, low_a);

    Uxn_deo_dispatch(uxn, addr);
    Uxn_deo_dispatch(uxn, addr + 1);


  } else {
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_dev_write(uxn, addr, a);

    Uxn_deo_dispatch(uxn, addr);
  }

  return pc;
}

/**
 * @brief Add: ADD (a b -- a+b)
 *
 * Pushes the sum of the two values at the top of the stack.
 */
Short op_add(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Short a = (high_a << 8) | low_a;
    Short b = (high_b << 8) | low_b;
    Short sum = a + b;

    Uxn_push(uxn, sum >> 8, return_mode);
    Uxn_push(uxn, sum & 0xff, return_mode);
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, a + b, return_mode);
  }

  return pc;
}

/**
 * @brief Subtract: SUB (a b -- a-b)
 *
 * Pushes the difference of the first value minus the second, to the top of the
 * stack.
 */
Short op_sub(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Short a = (high_a << 8) | low_a;
    Short b = (high_b << 8) | low_b;
    Short diff = a - b;

    Uxn_push(uxn, diff >> 8, return_mode);
    Uxn_push(uxn, diff & 0xff, return_mode);
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, a - b, return_mode);
  }

  return pc;
}

/**
 * @brief Multiply: MUL (a b -- a*b)
 *
 * Pushes the product of the first and second values at the top of the stack.
 */
Short op_mul(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Short a = (high_a << 8) | low_a;
    Short b = (high_b << 8) | low_b;
    Short prod = a * b;

    Uxn_push(uxn, prod >> 8, return_mode);
    Uxn_push(uxn, prod & 0xff, return_mode);
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, a * b, return_mode);
  }

  return pc;
}

/**
 * @brief Divide: DIV (a b -- a/b)
 *
 * Pushes the quotient of the first value over the second, to the top of the
 * stack. Division by zero pushes zero to the stack. The rounding direction is toward zero.
 */
Short op_div(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Short a = (high_a << 8) | low_a;
    Short b = (high_b << 8) | low_b;

    if (b == 0) {
      Uxn_push(uxn, 0, return_mode);
      Uxn_push(uxn, 0, return_mode);
    } else {
      Short quot = a / b;
      Uxn_push(uxn, quot >> 8, return_mode);
      Uxn_push(uxn, quot & 0xff, return_mode);
    }
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    if (b == 0) {
      Uxn_push(uxn, 0, return_mode);
    } else {
      Uxn_push(uxn, a / b, return_mode);
    }
  }

  return pc;
}

/**
 * @brief And: AND (a b -- a&b)
 * 
 * Pushes the bitwise AND of the two values at the top of the stack.
 */
Short op_and(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, high_a & high_b, return_mode);
    Uxn_push(uxn, low_a & low_b, return_mode);
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, a & b, return_mode);
  }

  return pc;
}

/**
 * @brief Or: OR (a b -- a|b)
 *
 * Pushes the bitwise OR of the two values at the top of the stack.
 */
Short op_ora(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, high_a | high_b, return_mode);
    Uxn_push(uxn, low_a | low_b, return_mode);
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, a | b, return_mode);
  }

  return pc;
}

/**
 * @brief Exclusive Or: EOR (a b -- a^b)
 * 
 * Pushes the bitwise XOR of the two values at the top of the stack.
 */
Short op_eor(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Byte low_b = Uxn_pop(uxn, return_mode);
    Byte high_b = Uxn_pop(uxn, return_mode);
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, high_a ^ high_b, return_mode);
    Uxn_push(uxn, low_a ^ low_b, return_mode);
  } else {
    Byte b = Uxn_pop(uxn, return_mode);
    Byte a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Uxn_push(uxn, a ^ b, return_mode);
  }

  return pc;
}

/**
 * @brief Shift: SFT (a shift8 -- c)
 *
 * Shifts the bits of the second value of the stack to the left or right,
 * depending on the control value at the top of the stack. The high nibble of
 * the control value indicates how many bits to shift left, the low nibble how
 * many bits to shift right. The rightward shift is done first.
 */
Short op_sft(Uxn* uxn, Short pc, bool keep_mode, bool return_mode, bool short_mode) {
  Stack* stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Byte shift = Uxn_pop(uxn, return_mode);

  Byte n_left = high_nibble(shift);
  Byte n_right = low_nibble(shift);

  if (short_mode) {
    Byte low_a = Uxn_pop(uxn, return_mode);
    Byte high_a = Uxn_pop(uxn, return_mode);

    if (keep_mode) Stack_set_ptr(stack, ptr);

    Short a = (high_a << 8) | low_a;
    Short result = (a >> n_right) << n_left;

    Uxn_push(uxn, result >> 8, return_mode);
    Uxn_push(uxn, result & 0xff, return_mode);
  } else {
    Byte a = Uxn_pop(uxn, return_mode);
    
    if (keep_mode) Stack_set_ptr(stack, ptr);

    Byte result = (a >> n_right) << n_left;

    Uxn_push(uxn, result, return_mode);
  }

  return pc;
}

bool Uxn_eval(Uxn* uxn, Short pc) {
  Byte full_op = uxn->ram[pc];
  pc += 1;

  bool continue_execution = true;

  while (continue_execution) {

    Byte keep_mode = is_keep_mode(full_op);
    Byte ret_mode = is_return_mode(full_op);
    Byte short_mode = is_short_mode(full_op);
    Byte op = opcode(full_op);
    
    // Immediate ops
    switch (full_op) {
      case 0x00: continue_execution = false; break; // BRK
      case 0x20: pc = op_jci(uxn, pc); break;
      case 0x40: pc = op_jmi(uxn, pc); break;
      case 0x60: pc = op_jsi(uxn, pc); break;
      case 0x80: pc = op_lit(uxn, pc, false, false); break;
      case 0xa0: pc = op_lit(uxn, pc, false, true); break;
      case 0xc0: pc = op_lit(uxn, pc, true, false); break;
      case 0xe0: pc = op_lit(uxn, pc, true, true); break;
      default: break;
    }

    switch(op) {
      case 0x01: pc = op_inc(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x02: pc = op_pop(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x03: pc = op_nip(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x04: pc = op_swp(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x05: pc = op_rot(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x06: pc = op_dup(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x07: pc = op_ovr(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x08: pc = op_equ(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x09: pc = op_neq(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x0a: pc = op_gth(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x0b: pc = op_lth(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x0c: pc = op_jmp(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x0d: pc = op_jcn(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x0e: pc = op_jsr(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x0f: pc = op_sth(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x10: pc = op_ldz(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x11: pc = op_stz(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x12: pc = op_ldr(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x13: pc = op_str(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x14: pc = op_lda(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x15: pc = op_sta(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x16: pc = op_dei(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x17: pc = op_deo(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x18: pc = op_add(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x19: pc = op_sub(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x1a: pc = op_mul(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x1b: pc = op_div(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x1c: pc = op_and(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x1d: pc = op_ora(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x1e: pc = op_eor(uxn, pc, keep_mode, ret_mode, short_mode); break;
      case 0x1f: pc = op_sft(uxn, pc, keep_mode, ret_mode, short_mode); break;
      default: break;
    }
  }
}

void Uxn_dump(Uxn* uxn) {
  if (uxn) {
    Stack_dump(uxn->work, "WORK");
    Stack_dump(uxn->ret, "RET");
  }
}