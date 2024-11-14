#include <stdio.h>

#include "common.h"
#include "uxn.h"
#include "device/system.h"

int main(int argc, const char* argv[]) {
  Uxn* uxn = Uxn_new();

  system_boot(uxn, "./opctest.rom");  

  Uxn_eval(uxn, RESET_VECTOR);
  

  Uxn_delete(uxn);

  return 0;
}

Byte Uxn_dei_dispatch(Uxn* uxn, Byte addr) {
  const Byte page = addr & 0x0f;
  switch (page) {
    case 0x00: return system_dei(uxn, addr);
    default: return Uxn_dev_read(uxn, addr);
  }
}

void Uxn_deo_dispatch(Uxn* uxn, Byte addr) {
  const Byte page = addr & 0x0f;
  switch (page) {
    case 0x00: return system_deo(uxn, addr);
    default: return;
  }
}