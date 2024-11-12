#include <stdio.h>

#include "common.h"
#include "uxn.h"
#include "debug.h"

int main(int argc, const char* argv[]) {
  Uxn uxn;
  initUxn(&uxn);

  bool should_continue = true;
  Short pc = START_PC;

  while (should_continue) {
    //should_continue = uxn_eval(&uxn, pc);

    uxn_push_work(&uxn, 0x80);
    uxn_push_ret(&uxn, 0x80);

    printf("\nBefore pop\n\n");

    dumpUxn(&uxn);

    printf("\nAfter pop\n\n");
    uxn_pop_work(&uxn);

    dumpUxn(&uxn);


    should_continue = false;
  }



  return 0;
}