#include "greatest.h"
#include "../src/common.h"
#include "../src/uxn.h"

SUITE_EXTERN(stack);
SUITE_EXTERN(uxn);

GREATEST_MAIN_DEFS();

void Uxn_dei_dispatch(Uxn* uxn, Byte addr) { }
Byte Uxn_deo_dispatch(Uxn* uxn, Byte addr) { return 0; }

int main(int argc, char **argv) {
  GREATEST_MAIN_BEGIN();
  RUN_SUITE(stack);
  RUN_SUITE(uxn);
  GREATEST_MAIN_END();
}