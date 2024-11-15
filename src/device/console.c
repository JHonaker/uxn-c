#include "console.h"

#include "../uxn.h"

#include <stdio.h>

int console_input_event(Uxn* uxn, Byte c, Byte type) {
  Uxn_dev_write(uxn, CONSOLE_READ_PORT, c);
  Uxn_dev_write(uxn, CONSOLE_TYPE_PORT, type);

  Byte high = Uxn_dev_read(uxn, CONSOLE_ADDR_PORT);
  Byte low = Uxn_dev_read(uxn, CONSOLE_ADDR_PORT + 1);
  Short vector_addr = (high << 8) | low;

  return Uxn_eval(uxn, vector_addr);
}

void console_display_write(Uxn* uxn) {
  Byte write_byte = Uxn_dev_read(uxn, CONSOLE_WRITE_PORT);
  fputc(write_byte, stdout);
  fflush(stdout);
}

void console_display_error(Uxn* uxn) {
  Byte error_byte = Uxn_dev_read(uxn, CONSOLE_ERROR_PORT);
  fputc(error_byte, stderr);
  fflush(stderr);
}

void console_deo(Uxn* uxn, Byte addr) {
  switch (addr) {
    case CONSOLE_WRITE_PORT:
      console_display_write(uxn);
      break;
    case CONSOLE_ERROR_PORT:
      console_display_error(uxn);
      break;
    default:
      break;
  }
}