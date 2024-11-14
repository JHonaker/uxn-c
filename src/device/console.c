#include "console.h"

#include "../uxn.h"

#include <stdio.h>

int console_input_event(Uxn* uxn, Byte c, Byte type) {
  Uxn_dev_write(uxn, CONSOLE_ADDR_READ, c);
  Uxn_dev_write(uxn, CONSOLE_ADDR_TYPE, type);

  Byte high = Uxn_dev_read(uxn, CONSOLE_ADDR_ADDR);
  Byte low = Uxn_dev_read(uxn, CONSOLE_ADDR_ADDR + 1);
  Short vector_addr = (high << 8) | low;

  return Uxn_eval(uxn, vector_addr);
}

void console_display_write(Uxn* uxn) {
  Byte write_byte = Uxn_dev_read(uxn, CONSOLE_ADDR_WRITE);
  fputc(write_byte, stdout);
  fflush(stdout);
}

void console_display_error(Uxn* uxn) {
  Byte error_byte = Uxn_dev_read(uxn, CONSOLE_ADDR_ERROR);
  fputc(error_byte, stderr);
  fflush(stderr);
}

void console_deo(Uxn* uxn, Byte addr) {
  switch (addr) {
    case CONSOLE_ADDR_WRITE:
      console_display_write(uxn);
      break;
    case CONSOLE_ADDR_ERROR:
      console_display_error(uxn);
      break;
    default:
      break;
  }
}