#include <stdio.h>
#include <raylib.h>

#include "common.h"
#include "uxn.h"
#include "device/system.h"
#include "device/console.h"
#include "device/screen.h"


void handle_keyboard(Uxn* uxn, RaylibScreen* screen) {
  KeyboardKey key = GetKeyPressed();
  while (key) {
    console_input_event(uxn, key, CONSOLE_TYPE_STDIN);
    key = GetKeyPressed();
  }
}
void handle_input(Uxn* uxn, RaylibScreen* screen) {
  handle_keyboard(uxn, screen);
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

int main(int argc, const char* argv[]) {
  Uxn* uxn = Uxn_new();

  RaylibScreen* screen = screen_new(900, 720, 1);

  screen_redraw(uxn, screen);

  system_boot(uxn, "./test.rom");  

  bool continue_execution = true;

  Uxn_eval(uxn, RESET_VECTOR);
  
  Short screen_vector = 0;

  while (continue_execution) {
    screen_vector = Uxn_dev_read(uxn, SCREEN_VECTOR_PORT);
    Uxn_eval(uxn, screen_vector);
    screen_redraw(uxn, screen);

    handle_input(uxn, screen);

    continue_execution = Uxn_dev_read(uxn, SYSTEM_STATE_PORT) == 0;
  }
  
  screen_delete(screen);
  Uxn_delete(uxn);

  return 0;
}

