#include "uxn.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>

inline int high_nibble(Byte byte) {
  return (byte & 0xf0) >> 4;
}

inline int low_nibble(Byte byte) {
  return byte & 0x0f;
}

struct Uxn {
  Byte ram[RAM_SIZE];
  Stack* work;
  Stack* ret;
};

void Uxn_init(Uxn* uxn) {
  if (uxn) {
    *uxn = (Uxn) {
      .ram = { 0 },
      .work = Stack_new(),
      .ret = Stack_new()
    };
  }
}

void Uxn_destroy(Uxn* uxn) {
  if (uxn) {
    for (int i = 0; i < RAM_SIZE; i++) {
      uxn->ram[i] = 0;
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

  switch (op) {
    case 0x80: { // LIT ( -- a)
      Byte literal_value = Uxn_peek_work_offset(uxn, 1);
      Uxn_push_work(uxn, literal_value);
      return true;
    }
    case 0x00: { // BRK ( -- )
      return true;
    }
    case 0x01: { // INC (a -- a+1)
      Byte popped = Uxn_pop_work(uxn);
      Uxn_push_work(uxn, popped + 1);
      return true;
    }
    // Stack manipulators
    case 0x02: { // POP (a -- )
      Uxn_pop_work(uxn);
      return true;
    }
    case 0x03: { // NIP (a b -- b)
      Byte b = Uxn_pop_work(uxn);
      Uxn_pop_work(uxn);
      Uxn_push_work(uxn, b);
      return true;
    }
    case 0x04: { // SWP (a b -- b a)
      Byte b = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      Uxn_push_work(uxn, b);
      Uxn_push_work(uxn, a);
      return true;
    }
    case 0x05: { // ROT (a b c -- b c a)
      Byte c = Uxn_pop_work(uxn);
      Byte b = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      Uxn_push_work(uxn, b);
      Uxn_push_work(uxn, c);
      Uxn_push_work(uxn, a);
      return true;
    }
    case 0x06: { // DUP (a -- a a)
      Byte a = Uxn_peek_work(uxn);
      Uxn_push_work(uxn, a);
      return true;
    }
    case 0x07: { // OVR (a b -- a b a)
      Byte a = Uxn_peek_work_offset(uxn, 1);
      Uxn_push_work(uxn, a);
      return true;
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
      return true;
    }
    case 0x09: { // NEQ (a b -- bool8)
      Byte b = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      if (a != b) {
        Uxn_push_work(uxn, 0x01);
      } else {
        Uxn_push_work(uxn, 0x00);
      }
      return true;
    }
    case 0x0a: { // GTH (a b -- bool8)
      Byte b = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      if (a > b) {
        Uxn_push_work(uxn, 0x01);
      } else {
        Uxn_push_work(uxn, 0x00);
      }
      return true;
    }
    case 0x0b: { // LTH (a b -- bool8)
      Byte b = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      if (a < b) {
        Uxn_push_work(uxn, 0x01);
      } else {
        Uxn_push_work(uxn, 0x00);
      }
      return true;
    }
    // Control flow
    case 0x0c: // JMP
      break;
    case 0x0d: // JCN
      break;
    case 0x0e: // JSR
      break;
    case 0x0f: // STH (a -- | a)
      Byte a = Uxn_pop_work(uxn);
      Uxn_push_ret(uxn, a);
      return true;
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
    case 0x18: { // ADD
      Byte b = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      Uxn_push_work(uxn, a + b);
      return true;
    }
    case 0x19: { // SUB
      Byte b = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      Uxn_push_work(uxn, a - b);
      return true;
    }
    case 0x1a: { // MUL
      Byte b = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      Uxn_push_work(uxn, a * b);
      return true;
    }
    case 0x1b: { // DIV
      Byte b = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      Uxn_push_work(uxn, Byte_div(a, b));
      return true;
    }
    // Bitwise operations
    case 0x1c: { // AND (a b -- a & b)
      Byte b = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      Uxn_push_work(uxn, a & b);
      return true;
    }
    case 0x1d: { // ORA
      Byte b = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      Uxn_push_work(uxn, a | b);
      return true;
    }
    case 0x1e: { // EOR
      Byte b = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      Uxn_push_work(uxn, a ^ b);
      return true;
    }
    case 0x1f: { // SFT
      Byte shiftByte = Uxn_pop_work(uxn);
      Byte a = Uxn_pop_work(uxn);
      int left_shift = high_nibble(shiftByte);
      int right_shift = low_nibble(shiftByte);
      Uxn_push_work(uxn, (a >> right_shift) << left_shift);
      return true;
    }
  }
}