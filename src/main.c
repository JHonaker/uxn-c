#include <stdio.h>

#include "common.h"
#include "uxn.h"

int main(int argc, const char* argv[]) {
  Uxn* uxn = Uxn_new();

  bool should_continue = true;
  Short pc = RESET_VECTOR;

  while (should_continue) {
    //should_continue = uxn_eval(&uxn, pc);

    Uxn_push_work(uxn, 0x80);
    Uxn_push_ret(uxn, 0x80);

    printf("\nBefore pop\n\n");

    Uxn_dump(uxn);

    printf("\nAfter pop\n\n");
    Uxn_pop_work(uxn);

    Uxn_dump(uxn);


    should_continue = false;
  }



  return 0;
}

Byte Uxn_dei_dispatch(Byte page) {
  return 0;
}

void Uxn_deo_dispatch(Byte page, Byte value) {
  return;
}