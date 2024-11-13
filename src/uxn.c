#include "uxn.h"
#include <stdio.h>
#include <stdlib.h>

extern Byte Uxn_dei_dispatch(Uxn* uxn, Byte addr);
extern void Uxn_deo_dispatch(Uxn* uxn, Byte addr, Byte value);

int high_nibble(Byte byte) {
  return (byte & 0xf0) >> 4;
}

int low_nibble(Byte byte) {
  return byte & 0x0f;
}

struct Uxn {
  Byte ram[RAM_SIZE];
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
    for (int i = 0; i < RAM_SIZE; i++) {
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

// Memory operations

Byte Uxn_read_byte(Uxn* uxn, Short address) {
  return uxn->ram[address];
}

Short Uxn_read_short(Uxn* uxn, Short address) {
  return (uxn->ram[address] << 8) | uxn->ram[address + 1];
}

void Uxn_write_byte(Uxn* uxn, Short address, Byte value) {
  uxn->ram[address] = value;
}

void Uxn_write_short(Uxn* uxn, Short address, Short value) {
  uxn->ram[address] = (value & 0xff00) >> 8;
  uxn->ram[address + 1] = value & 0x00ff;
}

// Device operations

Byte Uxn_page_read_byte(Uxn* uxn, Byte addr) {
  return uxn->dev[addr];
}

Short Uxn_page_read_short(Uxn* uxn, Byte addr) {
  return (uxn->dev[addr] << 8) | uxn->dev[addr + 1];
}

void Uxn_page_write_byte(Uxn* uxn, Byte addr, Byte value) {
  uxn->dev[addr] = value;
}

void Uxn_page_write_short(Uxn* uxn, Byte addr, Short value) {
  uxn->dev[addr] = (value & 0xff00) >> 8;
  uxn->dev[addr + 1] = value & 0x00ff;
}


// Op codes

Byte Byte_div(Byte a, Byte b) {
  if (b == 0) {
    return 0;
  }
  
  // To round toward zero
  int sign = ((a < 0) ^ (b < 0)) ? -1 : 1;
  return sign * (abs(a) / abs(b));
}

bool Uxn_eval(Uxn* uxn, Short pc) {
  Byte op = uxn->ram[pc];

  bool continue_execution = true;
  int pc_increment = 1;

  while (continue_execution) {
    switch (op) {
      
      case 0x80: { // LIT ( -- a)
        Byte literal_value = Uxn_read_byte(uxn, pc + 1);
        Uxn_push_work(uxn, literal_value);
        pc_increment = 2;
        break;
      }
      case 0x00: { // BRK ( -- )
        continue_execution = false;
        break;
      }
      case 0x01: { // INC (a -- a+1)
        Byte popped = Uxn_pop_work(uxn);
        Uxn_push_work(uxn, popped + 1);
        break;
      }
      // Stack manipulators
      case 0x02: { // POP (a -- )
        Uxn_pop_work(uxn);
        break;
      }
      case 0x03: { // NIP (a b -- b)
        Byte b = Uxn_pop_work(uxn);
        Uxn_pop_work(uxn);
        Uxn_push_work(uxn, b);
        break;
      }
      case 0x04: { // SWP (a b -- b a)
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        Uxn_push_work(uxn, b);
        Uxn_push_work(uxn, a);
        break;
      }
      case 0x05: { // ROT (a b c -- b c a)
        Byte c = Uxn_pop_work(uxn);
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        Uxn_push_work(uxn, b);
        Uxn_push_work(uxn, c);
        Uxn_push_work(uxn, a);
        break;
      }
      case 0x06: { // DUP (a -- a a)
        Byte a = Uxn_peek_work(uxn);
        Uxn_push_work(uxn, a);
        break;
      }
      case 0x07: { // OVR (a b -- a b a)
        Byte a = Uxn_peek_work_offset(uxn, 1);
        Uxn_push_work(uxn, a);
        break;
      }
      // Logical operators
      case 0x08: { // EQU (a b -- bool8)
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        if (a == b) {
          Uxn_push_work(uxn, 0x01);
        } else {
          Uxn_push_work(uxn, 0x00);
        }
        break;
      }
      case 0x09: { // NEQ (a b -- bool8)
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        if (a != b) {
          Uxn_push_work(uxn, 0x01);
        } else {
          Uxn_push_work(uxn, 0x00);
        }
        break;
      }
      case 0x0a: { // GTH (a b -- bool8)
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        if (a > b) {
          Uxn_push_work(uxn, 0x01);
        } else {
          Uxn_push_work(uxn, 0x00);
        }
        break;
      }
      case 0x0b: { // LTH (a b -- bool8)
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        if (a < b) {
          Uxn_push_work(uxn, 0x01);
        } else {
          Uxn_push_work(uxn, 0x00);
        }
        break;
      }
      // Control flow
      case 0x0c: { // JMP (addr -- )
        SignedByte rel_dist = (SignedByte) Uxn_pop_work(uxn);
        pc_increment = rel_dist;
        break;
      }
      case 0x0d: { // JCN (cond8 addr -- )
        Byte addr = Uxn_pop_work(uxn);
        Byte cond = Uxn_pop_work(uxn);
        if (cond) {
          // If the cond byte is not 00, moves the PC by the signed value of the
          // addr byte
          pc_increment = (SignedByte) addr;
        }
        break;
      }
      case 0x0e: { // JSR (addr -- | ret16)
        Uxn_push_ret(uxn, pc + 2);
        Short rel_addr = Uxn_read_short(uxn, pc + 1);
        pc_increment = (SignedShort) rel_addr;
        break;
      }
      case 0x0f: // STH (a -- | a)
        Byte a = Uxn_pop_work(uxn);
        Uxn_push_ret(uxn, a);
        break;
      // Instant jumps
      case 0x20: { // JCI (cond8 -- )
        Byte cond = Uxn_pop_work(uxn);
        if (cond) {
          pc_increment = (SignedShort) Uxn_read_short(uxn, pc + 1);
        } else {
          pc_increment = 2;
        }
        break;
      }
      case 0x40: { // JMI ( -- )
        Short rel_addr = Uxn_read_short(uxn, pc + 1);
        pc_increment = (SignedShort) rel_addr;
        break;
      }
      case 0x60: { // JSI ( -- )

        Short rel_addr = Uxn_read_short(uxn, pc + 1);
        Uxn_push_ret(uxn, pc);
        pc_increment = (SignedShort) rel_addr;
        break;
      }
      // Memory operations
      case 0x10: { // LDZ (addr8 -- value)
        Byte addr = Uxn_pop_work(uxn);
        Byte value = Uxn_read_byte(uxn, addr);
        Uxn_push_work(uxn, value);
        break;
      }
      case 0x11: { // STZ (value addr8 -- )
        Byte addr = Uxn_pop_work(uxn);
        Byte value = Uxn_pop_work(uxn);
        Uxn_write_byte(uxn, addr, value);
        break;
      }
      case 0x12: { // LDR (addr8 -- value)
        SignedByte rel_addr = Uxn_pop_work(uxn);
        Byte value = Uxn_read_byte(uxn, pc + rel_addr);
        Uxn_push_work(uxn, value);
        break;
      }
      case 0x13: { // STR (value addr8 -- )
        SignedByte rel_addr = Uxn_pop_work(uxn);
        Byte value = Uxn_pop_work(uxn);
        Uxn_write_byte(uxn, pc + rel_addr, value);
        break;
      }
      case 0x14: { // LDA (addr16 -- value)
        Byte low_byte = Uxn_pop_work(uxn);
        Byte high_byte = Uxn_pop_work(uxn);
        Short addr = (high_byte << 8) | low_byte;
        Byte value = Uxn_read_byte(uxn, addr);
        Uxn_push_work(uxn, value);
        break;
      }
      case 0x15: { // STA (value addr16 -- )
        Byte low_byte = Uxn_pop_work(uxn);
        Byte high_byte = Uxn_pop_work(uxn);
        Short addr = (high_byte << 8) | low_byte;
        Byte value = Uxn_pop_work(uxn);
        Uxn_write_byte(uxn, addr, value);
        break;
      }
      case 0x16: { // DEI (device8 -- value)
        Byte device_addr = Uxn_pop_work(uxn);
        Byte value = Uxn_dei_dispatch(uxn, device_addr);
        Uxn_push_work(uxn, value);
        break;
      }
      case 0x17: { // DEO (value device8 -- value)
        Byte device_addr = Uxn_pop_work(uxn);
        Byte value = Uxn_pop_work(uxn);
        Uxn_deo_dispatch(uxn, device_addr, value);
        break;
      }
      // Arithmetic operations
      case 0x18: { // ADD
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        Uxn_push_work(uxn, a + b);
        break;
      }
      case 0x19: { // SUB
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        Uxn_push_work(uxn, a - b);
        break;
      }
      case 0x1a: { // MUL
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        Uxn_push_work(uxn, a * b);
        break;
      }
      case 0x1b: { // DIV
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        Uxn_push_work(uxn, Byte_div(a, b));
        break;
      }
      // Bitwise operations
      case 0x1c: { // AND (a b -- a & b)
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        Uxn_push_work(uxn, a & b);
        break;
      }
      case 0x1d: { // ORA
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        Uxn_push_work(uxn, a | b);
        break;
      }
      case 0x1e: { // EOR
        Byte b = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        Uxn_push_work(uxn, a ^ b);
        break;
      }
      case 0x1f: { // SFT
        Byte shiftByte = Uxn_pop_work(uxn);
        Byte a = Uxn_pop_work(uxn);
        int left_shift = high_nibble(shiftByte);
        int right_shift = low_nibble(shiftByte);
        Uxn_push_work(uxn, (a >> right_shift) << left_shift);
        break;
      }
    }

    pc += pc_increment;
  }
}

bool Uxn_step(Uxn* uxn) {
  if (uxn) {
    if (Uxn_eval(uxn, RESET_VECTOR)) {
      return true;
    }
  }
}

void Uxn_dump(Uxn* uxn) {
  if (uxn) {
    Stack_dump(uxn->work, "WORK");
    Stack_dump(uxn->ret, "RET");
  }
}