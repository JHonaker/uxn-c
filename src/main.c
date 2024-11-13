#include <stdio.h>

#include "common.h"
#include "uxn.h"

int main(int argc, const char* argv[]) {
  Uxn* uxn = Uxn_new();

  Short pc = RESET_VECTOR;

  Uxn_push_work(uxn, 0x80);
  Uxn_push_ret(uxn, 0x80);

  printf("\nBefore pop\n\n");

  Uxn_dump(uxn);

  printf("\nAfter pop\n\n");
  Uxn_pop_work(uxn);

  Uxn_dump(uxn);

  Uxn_pop_ret(uxn);

  Uxn_mem_write(uxn, RESET_VECTOR, 0x00);

  Uxn_eval(uxn, RESET_VECTOR);
  printf("\n After eval! \n\n");

  Uxn_delete(uxn);

  return 0;
}

Byte Uxn_dei_dispatch(Uxn* uxn, Byte addr) {
  return Uxn_dev_read(uxn, addr);
}

void Uxn_deo_dispatch(Uxn* uxn, Byte addr) {
  return;
}