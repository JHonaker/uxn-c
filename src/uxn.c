#include "uxn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_ADDR(page, addr)                                                  \
  ((page) * RAM_PAGE_SIZE + (addr))

extern Byte uxn_dei_dispatch(Uxn *uxn, Byte addr);
extern void uxn_deo_dispatch(Uxn *uxn, Byte addr);

int high_nibble(Byte byte) { return (byte & 0xf0) >> 4; }

int low_nibble(Byte byte) { return byte & 0x0f; }

bool is_keep_mode(Byte op) { return (op & 0x80) == 0x80; }

bool is_return_mode(Byte op) { return (op & 0x40) == 0x40; }

bool is_short_mode(Byte op) { return (op & 0x20) == 0x20; }

Byte opcode(Byte op) { return op & 0x1f; }

struct Uxn {
  Byte ram[RAM_PAGE_SIZE * RAM_PAGES];
  Byte dev[DEV_PAGE_SIZE];
  Stack *work;
  Stack *ret;
  void *screen;
  void *open_files;
};

void uxn_init(Uxn *uxn, void *screen) {
  if (uxn) {
    *uxn = (Uxn){.ram = {0},
                 .dev = {0},
                 .work = Stack_new(),
                 .ret = Stack_new(),
                 .screen = screen,
                 .open_files = NULL};
  }
}

void uxn_destroy(Uxn *uxn) {
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

void *uxn_get_screen(Uxn *uxn) { return uxn->screen; }

void *uxn_get_open_files(Uxn *uxn) { return uxn->open_files; }
void uxn_set_open_files(Uxn *uxn, void *files) { uxn->open_files = files; }

Uxn *uxn_new(void *screen) {
  Uxn *uxn = malloc(sizeof(Uxn));
  uxn_init(uxn, screen);
  return uxn;
}

void uxn_delete(Uxn *uxn) {
  if (uxn) {
    uxn_destroy(uxn);
    free(uxn);
  }
}

// Stack operations

void uxn_stack_zero(Uxn *uxn) {
  Stack_set_ptr(uxn->work, 0);
  Stack_set_ptr(uxn->ret, 0);
}

Byte uxn_work_ptr(Uxn *uxn) { return Stack_get_ptr(uxn->work); }

Byte uxn_ret_ptr(Uxn *uxn) { return Stack_get_ptr(uxn->ret); }

void uxn_set_work_ptr(Uxn *uxn, Byte ptr) { Stack_set_ptr(uxn->work, ptr); }

void uxn_set_ret_ptr(Uxn *uxn, Byte ptr) { Stack_set_ptr(uxn->ret, ptr); }

void uxn_push_work(Uxn *uxn, Byte value) { Stack_push(uxn->work, value); }

Byte uxn_pop_work(Uxn *uxn) { return Stack_pop(uxn->work); }

Byte uxn_peek_work_offset(Uxn *uxn, Byte offset) {
  return Stack_peek_offset(uxn->work, offset);
}

Byte uxn_peek_work(Uxn *uxn) { return Stack_peek(uxn->work); }

void uxn_push_ret(Uxn *uxn, Byte value) { Stack_push(uxn->ret, value); }

Byte uxn_pop_ret(Uxn *uxn) { return Stack_pop(uxn->ret); }

Byte uxn_peek_ret(Uxn *uxn) { return Stack_peek(uxn->ret); }

void uxn_push(Uxn *uxn, Byte value, bool to_return_stack) {
  if (to_return_stack) {
    uxn_push_ret(uxn, value);
  } else {
    uxn_push_work(uxn, value);
  }
}

Byte uxn_pop(Uxn *uxn, bool from_return_stack) {
  if (from_return_stack) {
    return uxn_pop_ret(uxn);
  } else {
    return uxn_pop_work(uxn);
  }
}

Short uxn_pop_short(Uxn *uxn, bool from_return_stack) {
  Byte low = uxn_pop(uxn, from_return_stack);
  Byte high = uxn_pop(uxn, from_return_stack);
  return (high << 8) | low;
}

void uxn_push_short(Uxn *uxn, Short value, bool to_return_stack) {
  uxn_push(uxn, value >> 8, to_return_stack);
  uxn_push(uxn, value & 0xff, to_return_stack);
}

// Memory operations

Byte uxn_page_read(Uxn *uxn, Short page, size_t addr) {
  return uxn->ram[PAGE_ADDR(page, addr)];
}

void uxn_page_load(Uxn *uxn, Byte program[], unsigned long size, size_t page,
                   size_t addr) {
  size_t idx = PAGE_ADDR(page, addr);
  memcpy(&uxn->ram[idx], program, size);
}

void uxn_page_write(Uxn *uxn, Short page, size_t addr, Byte value) {
  uxn->ram[PAGE_ADDR(page, addr)] = value;
}
void uxn_mem_zero(Uxn *uxn, bool soft) {
  for (int i = (soft ? RESET_VECTOR : 0); i < RAM_PAGE_SIZE * RAM_PAGES; i++) {
    uxn->ram[i] = 0;
  }
}

void uxn_mem_load(Uxn *uxn, Byte program[], unsigned long size, size_t addr) {
  uxn_page_load(uxn, program, size, 0, addr);
}

Byte uxn_mem_read(Uxn *uxn, size_t addr) { return uxn->ram[PAGE_ADDR(0, addr)]; }

void uxn_mem_buffer_read(Uxn *uxn, Short size, Byte buffer[size], size_t addr) {
  memcpy(buffer, &uxn->ram[addr], size);
}

Short uxn_mem_read_short(Uxn *uxn, size_t addr) {
  return (uxn->ram[PAGE_ADDR(0, addr)] << 8) | uxn->ram[PAGE_ADDR(0, addr + 1)];
}

void uxn_mem_write(Uxn *uxn, size_t addr, Byte value) {
  uxn->ram[PAGE_ADDR(0, addr)] = value;
}

void uxn_mem_write_short(Uxn *uxn, size_t addr, Short value) {
  uxn->ram[PAGE_ADDR(0, addr)] = value >> 8;
  uxn->ram[PAGE_ADDR(0, addr + 1)] = value & 0xff;
}

Byte uxn_zero_page_read(Uxn *uxn, Byte addr) {
  return uxn->ram[addr & (RESET_VECTOR - 1)];
}

Short uxn_zero_page_read_short(Uxn *uxn, Byte addr) {
  Byte high = uxn->ram[addr & (RESET_VECTOR - 1)];
  Byte low = uxn->ram[(addr + 1) & (RESET_VECTOR - 1)];
  return (high << 8) | low;
}

void uxn_zero_page_write(Uxn *uxn, Byte addr, Byte value) {
  uxn->ram[addr & (RESET_VECTOR - 1)] = value;
}

void uxn_zero_page_write_short(Uxn *uxn, Byte addr, Short value) {
  uxn->ram[addr & (RESET_VECTOR - 1)] = value >> 8;
  uxn->ram[(addr + 1) & (RESET_VECTOR - 1)] = value & 0xff;
}

// Device operations

void uxn_dev_zero(Uxn *uxn) {
  for (int i = 0; i < DEV_PAGE_SIZE; i++) {
    uxn->dev[i] = 0;
  }
}

Byte uxn_dev_read(Uxn *uxn, Byte addr) { return uxn->dev[addr]; }

Short uxn_dev_read_short(Uxn *uxn, Byte addr) {
  return (uxn->dev[addr & 0xff] << 8 | uxn->dev[(addr + 1) & 0xff]);
}

void uxn_dev_write(Uxn *uxn, Byte addr, Byte value) { uxn->dev[addr & 0xff] = value; }

void uxn_dev_write_short(Uxn *uxn, Byte addr, Short value) {
  uxn->dev[addr & 0xff] = (Byte)(value >> 8);
  uxn->dev[(addr + 1) & 0xff] = (Byte)(value & 0xff);
}

// Immediate Ops

/**
 * @brief Jump Instant: JMI ( -- )
 *
 * Moves the PC to a relative address at a distance equal to the next short in
 * memory. This opcode has no modes.
 */
Short op_jmi(Uxn *uxn, Short pc) {
  Short rel_addr = uxn_mem_read_short(uxn, pc);
  return pc + (SignedShort)rel_addr + 2;
}

/**
 * @brief Jump Stash Return Instant: JSI ( -- )
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
 * @brief Jump Conditional Instant: JCI (cond8 -- )
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
 * @brief Literal: LIT ( -- a)
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
 * @brief Increment: Inc (a -- a+1)
 *
 * Increments the top value of the stack by one.
 */
Short op_inc(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, a + 1, return_mode);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, a + 1, return_mode);
  }

  return pc;
}

/**
 * @brief Pop: POP (a -- )
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
 * @brief Nip: NIP (a b -- b)
 *
 * Removes the second value from the stack. This is useful to truncate a short
 * into a byte.
 */
Short op_nip(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, b, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, b, return_mode);
  }

  return pc;
}

/**
 * @brief Swap: SWP (a b -- b a)
 *
 * Exchanges the top two values on the stack.
 */
Short op_swp(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, b, return_mode);
    uxn_push_short(uxn, a, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, b, return_mode);
    uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * @brief Rotate: ROT (a b c -- b c a)
 *
 * Rotates three values at the top of the stack, to the left, wrapping around.
 */
Short op_rot(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short c = uxn_pop_short(uxn, return_mode);
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, b, return_mode);
    uxn_push_short(uxn, c, return_mode);
    uxn_push_short(uxn, a, return_mode);
  } else {
    Byte c = uxn_pop(uxn, return_mode);
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, b, return_mode);
    uxn_push(uxn, c, return_mode);
    uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * @brief Duplicate: DUP (a -- a a)
 *
 * Duplicates the value at the top of the stack.
 */
Short op_dup(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, a, return_mode);
    uxn_push_short(uxn, a, return_mode);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, a, return_mode);
    uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * @brief Over: OVR (a b -- a b a)
 *
 * Duplicates the second value at the top of the stack.
 */
Short op_ovr(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, a, return_mode);
    uxn_push_short(uxn, b, return_mode);
    uxn_push_short(uxn, a, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, a, return_mode);
    uxn_push(uxn, b, return_mode);
    uxn_push(uxn, a, return_mode);
  }

  return pc;
}

/**
 * @brief Equal: EQU (a b -- bool8)
 *
 * Pushes 01 to the stack if the two values at the top of the stack are equal,
 * 00 otherwise.
 */
Short op_equ(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

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
    Stack_set_ptr(stack, ptr);

  uxn_push(uxn, result ? 0x01 : 0x00, return_mode);

  return pc;
}

/**
 * @brief Not Equal: NEQ (a b -- bool8)
 *
 * Pushes 01 to the stack if the two values at the top of the stack are not
 * equal, 00 otherwise.
 */
Short op_neq(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

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
    Stack_set_ptr(stack, ptr);

  uxn_push(uxn, result ? 0x01 : 0x00, return_mode);

  return pc;
}

/**
 * @brief Greater Than: GTH (a b -- bool8)
 *
 * Pushes 01 to the stack if a > b, 00 otherwise.
 */
Short op_gth(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

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
    Stack_set_ptr(stack, ptr);

  uxn_push(uxn, result ? 0x01 : 0x00, return_mode);

  return pc;
}

/**
 * @brief Less Than: LTH (a b -- bool8)
 *
 * Pushes 01 to the stack if a < b, 00 otherwise.
 */
Short op_lth(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

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
    Stack_set_ptr(stack, ptr);

  uxn_push(uxn, result ? 0x01 : 0x00, return_mode);

  return pc;
}

/**
 * @brief Jump: JMP (addr -- )
 *
 * Move the PC by a relative distance equal to the signed byte on the top of the
 * stack, or to an absolute address in short mode.
 */
Short op_jmp(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    // Jump to absolute address
    Short addr = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    return addr;
  } else {
    // Jump by a relative address
    SignedByte rel_addr = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

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
Short op_jcn(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short addr = uxn_pop_short(uxn, return_mode);

    Byte test_byte = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    // In short mode, jump to an absolute address
    return test_byte != 0x00 ? addr : pc;
  } else {
    SignedByte rel_addr = uxn_pop(uxn, return_mode);
    Byte test_byte = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

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
Short op_jsr(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  uxn_push_short(uxn, pc, !return_mode);

  if (short_mode) {
    Short addr = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    return addr;
  } else {
    SignedByte rel_addr = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

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
Short op_sth(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, a, !return_mode);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, a, !return_mode);
  }

  return pc;
}

/**
 * @brief Load Zero Page: LDZ (addr8 -- a)
 *
 * Pushes the value at an address within the first 256 bytes of memeory, to the
 * top of the stack.
 */
Short op_ldz(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Byte addr = uxn_pop(uxn, return_mode);

  if (keep_mode)
    Stack_set_ptr(stack, ptr);

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
 * @brief Store Zero Page: STZ (a addr8 -- )
 *
 * Writes a value to an address within the first 256 bytes of memory.
 */
Short op_stz(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Byte addr = uxn_pop(uxn, return_mode);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_zero_page_write_short(uxn, addr, a);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_zero_page_write(uxn, addr, a);
  }

  return pc;
}

/**
 * @brief Load Relative: LDR (addr8 -- a)
 *
 * Pushes a value at a relative address in relation to the PC, within a range
 * between -128 and +127 bytes, to the top of the stack.
 */
Short op_ldr(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  SignedByte rel_addr = uxn_pop(uxn, return_mode);

  if (keep_mode)
    Stack_set_ptr(stack, ptr);

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
 * @brief Store Relative: STR (a addr8 -- )
 *
 * Writes a value to a relative address in relation to the PC, within a range
 * between -128 and +127 bytes.
 */
Short op_str(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  SignedByte rel_addr = uxn_pop(uxn, return_mode);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    Short store_addr = pc + rel_addr;
    uxn_mem_write_short(uxn, store_addr, a);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    Short store_addr = pc + rel_addr;

    uxn_mem_write(uxn, store_addr, a);
  }

  return pc;
}

/**
 * @brief Load Absolute: LDA (addr16 -- a)
 *
 * Pushes a value at an absolute address in memory to the top of the stack.
 */
Short op_lda(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Short addr = uxn_pop_short(uxn, return_mode);

  if (keep_mode)
    Stack_set_ptr(stack, ptr);

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
 * @brief Store Absolute: STA (a addr16 -- )
 *
 * Writes a value to an absolute address in memory.
 */
Short op_sta(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Short addr = uxn_pop_short(uxn, return_mode);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);
    
    uxn_mem_write_short(uxn, addr, a);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_mem_write(uxn, addr, a);
  }

  return pc;
}

/**
 * @brief Device Input: DEI (device8 -- a)
 *
 * Pushes a value from the device page to the top of the stack.
 * The target device might capture the reading to trigger an I/O event.
 */
Short op_dei(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Byte addr = uxn_pop(uxn, return_mode);

  if (keep_mode)
    Stack_set_ptr(stack, ptr);

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
 * @brief Device Output: DEO (a device8 -- )
 *
 * Writes a value to the device page. The target device might capture the
 * writing to trigger an I/O event.
 */
Short op_deo(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Byte addr = uxn_pop(uxn, return_mode);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_dev_write_short(uxn, addr, a);

    uxn_deo_dispatch(uxn, addr);
    uxn_deo_dispatch(uxn, addr + 1);

  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_dev_write(uxn, addr, a);

    uxn_deo_dispatch(uxn, addr);
  }

  return pc;
}

/**
 * @brief Add: ADD (a b -- a+b)
 *
 * Pushes the sum of the two values at the top of the stack.
 */
Short op_add(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    Short sum = a + b;

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, sum, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, a + b, return_mode);
  }

  return pc;
}

/**
 * @brief Subtract: SUB (a b -- a-b)
 *
 * Pushes the difference of the first value minus the second, to the top of the
 * stack.
 */
Short op_sub(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    Short diff = a - b;

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, diff, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, a - b, return_mode);
  }

  return pc;
}

/**
 * @brief Multiply: MUL (a b -- a*b)
 *
 * Pushes the product of the first and second values at the top of the stack.
 */
Short op_mul(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    Short prod = a * b;

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, prod, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, a * b, return_mode);
  }

  return pc;
}

/**
 * @brief Divide: DIV (a b -- a/b)
 *
 * Pushes the quotient of the first value over the second, to the top of the
 * stack. Division by zero pushes zero to the stack. The rounding direction is
 * toward zero.
 */
Short op_div(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

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
      Stack_set_ptr(stack, ptr);

    if (b == 0) {
      uxn_push(uxn, 0, return_mode);
    } else {
      uxn_push(uxn, a / b, return_mode);
    }
  }

  return pc;
}

/**
 * @brief And: AND (a b -- a&b)
 *
 * Pushes the bitwise AND of the two values at the top of the stack.
 */
Short op_and(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, a & b, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, a & b, return_mode);
  }

  return pc;
}

/**
 * @brief Or: OR (a b -- a|b)
 *
 * Pushes the bitwise OR of the two values at the top of the stack.
 */
Short op_ora(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, a | b, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, a | b, return_mode);
  }

  return pc;
}

/**
 * @brief Exclusive Or: EOR (a b -- a^b)
 *
 * Pushes the bitwise XOR of the two values at the top of the stack.
 */
Short op_eor(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  if (short_mode) {
    Short b = uxn_pop_short(uxn, return_mode);
    Short a = uxn_pop_short(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, a ^ b, return_mode);
  } else {
    Byte b = uxn_pop(uxn, return_mode);
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push(uxn, a ^ b, return_mode);
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
Short op_sft(Uxn *uxn, Short pc, bool keep_mode, bool return_mode,
             bool short_mode) {
  Stack *stack = return_mode ? uxn->ret : uxn->work;
  Byte ptr = Stack_get_ptr(stack);

  Byte shift = uxn_pop(uxn, return_mode);

  Byte n_left = high_nibble(shift);
  Byte n_right = low_nibble(shift);

  if (short_mode) {
    Short a = uxn_pop_short(uxn, return_mode);

    Short result = (a >> n_right) << n_left;

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    uxn_push_short(uxn, result, return_mode);
  } else {
    Byte a = uxn_pop(uxn, return_mode);

    if (keep_mode)
      Stack_set_ptr(stack, ptr);

    Byte result = (a >> n_right) << n_left;

    uxn_push(uxn, result, return_mode);
  }

  return pc;
}

bool uxn_eval(Uxn *uxn, Short pc) {

  if (!pc) return 1;

  bool continue_execution = true;

  while (continue_execution) {
    Byte full_op = uxn_mem_read(uxn, pc);
    pc += 1;

    Byte keep_mode = is_keep_mode(full_op);
    Byte ret_mode = is_return_mode(full_op);
    Byte short_mode = is_short_mode(full_op);
    Byte op = opcode(full_op);

    // Immediate ops
    switch (full_op) {
    case 0x00:
      continue_execution = false;
      continue; // BRK
    case 0x20:
      pc = op_jci(uxn, pc);
      continue;
    case 0x40:
      pc = op_jmi(uxn, pc);
      continue;
    case 0x60:
      pc = op_jsi(uxn, pc);
      continue;
    case 0x80:
      pc = op_lit(uxn, pc, false, false);
      continue;
    case 0xa0:
      pc = op_lit(uxn, pc, false, true);
      continue;
    case 0xc0:
      pc = op_lit(uxn, pc, true, false);
      continue;
    case 0xe0:
      pc = op_lit(uxn, pc, true, true);
      continue;
    default:
      break;
    }

    switch (op) {
    case 0x01:
      pc = op_inc(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x02:
      pc = op_pop(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x03:
      pc = op_nip(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x04:
      pc = op_swp(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x05:
      pc = op_rot(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x06:
      pc = op_dup(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x07:
      pc = op_ovr(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x08:
      pc = op_equ(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x09:
      pc = op_neq(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x0a:
      pc = op_gth(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x0b:
      pc = op_lth(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x0c:
      pc = op_jmp(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x0d:
      pc = op_jcn(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x0e:
      pc = op_jsr(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x0f:
      pc = op_sth(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x10:
      pc = op_ldz(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x11:
      pc = op_stz(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x12:
      pc = op_ldr(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x13:
      pc = op_str(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x14:
      pc = op_lda(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x15:
      pc = op_sta(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x16:
      pc = op_dei(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x17:
      pc = op_deo(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x18:
      pc = op_add(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x19:
      pc = op_sub(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x1a:
      pc = op_mul(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x1b:
      pc = op_div(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x1c:
      pc = op_and(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x1d:
      pc = op_ora(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x1e:
      pc = op_eor(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    case 0x1f:
      pc = op_sft(uxn, pc, keep_mode, ret_mode, short_mode);
      break;
    default:
      break;
    }
  }

  return 0;
}

void uxn_dump(Uxn *uxn) {
  if (uxn) {
    Stack_dump(uxn->work, "WST");
    Stack_dump(uxn->ret, "RST");
  }
}

void uxn_dump_page(Uxn *uxn, Short page) {
  if (uxn) {
    printf("Page %d\n", page);
    for (size_t i = 0; i < RAM_PAGE_SIZE; i++) {
      printf("%02x ", uxn->ram[PAGE_ADDR(page, i)]);
    }
    printf("\n\n");
  }
}