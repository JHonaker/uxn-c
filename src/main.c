#include <stdio.h>

#include "common.h"
#include "uxn.h"
#include "device/system.h"
#include "device/console.h"

int main(int argc, const char* argv[]) {
  Uxn* uxn = Uxn_new();

  system_boot(uxn, "./test.rom");  

  Uxn_eval(uxn, RESET_VECTOR);

  Uxn_delete(uxn);

  return 0;
}

Byte Uxn_dei_dispatch(Uxn* uxn, Byte addr) {
  const Byte page = addr & 0xf0;
  switch (page) {
    case 0x00: return system_dei(uxn, addr);
    default: return Uxn_dev_read(uxn, addr);
  }
}

void Uxn_deo_dispatch(Uxn* uxn, Byte addr) {
  const Byte page = addr & 0xf0;
  switch (page) {
    case 0x00: system_deo(uxn, addr); break;
    case 0x10: console_deo(uxn, addr); break;
    default: break;
  }
}