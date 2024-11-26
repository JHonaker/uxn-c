#include "../src/common.h"
#include "../src/uxn.h"
#include "greatest.h"

SUITE_EXTERN(stack);
SUITE_EXTERN(uxn);

GREATEST_MAIN_DEFS();

void uxn_dei_dispatch(Uxn *uxn, Byte addr) {}
Byte uxn_deo_dispatch(Uxn *uxn, Byte addr) { return 0; }

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(stack);
  RUN_SUITE(uxn);
  GREATEST_MAIN_END();
}