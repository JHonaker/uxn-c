#include "uxn.h"
#include "ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_ADDR(page, addr)                                                  \
  ((page) * RAM_PAGE_SIZE + (addr))

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

Byte uxn_mem_read(Uxn *uxn, size_t addr) { return uxn->ram[PAGE_ADDR(0, addr) & (RAM_PAGE_SIZE - 1)]; }

void uxn_mem_buffer_read(Uxn *uxn, Short size, Byte buffer[size], size_t addr) {
  memcpy(buffer, &uxn->ram[addr], size);
}

Short uxn_mem_read_short(Uxn *uxn, size_t addr) {
  return (uxn->ram[PAGE_ADDR(0, addr) & (RAM_PAGE_SIZE - 1)] << 8) | uxn->ram[PAGE_ADDR(0, addr + 1) & (RAM_PAGE_SIZE - 1)];
}

void uxn_mem_write(Uxn *uxn, size_t addr, Byte value) {
  uxn->ram[PAGE_ADDR(0, addr) & (RAM_PAGE_SIZE - 1)] = value;
}

void uxn_mem_write_short(Uxn *uxn, size_t addr, Short value) {
  uxn->ram[PAGE_ADDR(0, addr) & (RAM_PAGE_SIZE - 1)] = value >> 8;
  uxn->ram[PAGE_ADDR(0, addr + 1) & (RAM_PAGE_SIZE - 1)] = value & 0xff;
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