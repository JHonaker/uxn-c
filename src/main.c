#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "device/console.h"
#include "device/screen.h"
#include "device/system.h"
#include "uxn.h"

void handle_keyboard(Uxn *uxn) {
  KeyboardKey key = GetKeyPressed();
  while (key) {
    console_input_event(uxn, key, CONSOLE_TYPE_STDIN);
    key = GetKeyPressed();
  }
}
void handle_input(Uxn *uxn) { handle_keyboard(uxn); }

Byte Uxn_dei_dispatch(Uxn *uxn, Byte addr) {
  const Byte page = addr & 0xf0;
  switch (page) {
  case 0x00:
    return system_dei(uxn, addr);
  case 0x20:
    return screen_dei(uxn, addr);
  default:
    return Uxn_dev_read(uxn, addr);
  }
}

void Uxn_deo_dispatch(Uxn *uxn, Byte addr) {
  const Byte page = addr & 0xf0;
  switch (page) {
  case 0x00: {
    system_deo(uxn, addr);
    if (SYSTEM_RED_PORT <= addr && addr <= SYSTEM_BLUE_PORT + 1) {
      screen_change_palette(uxn);
    }
    break;
  }
  case 0x10:
    console_deo(uxn, addr);
    break;
  case 0x20:
    screen_deo(uxn, addr);
    break;
  default:
    break;
  }
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    printf("Usage: %s <rom>\n", argv[0]);
    return 1;
  }

  int scale = 1;

  int opt;
  while ((opt = getopt(argc, argv, "s:")) != -1) {
    switch (opt) {
    case 's':
      scale = atoi(optarg);
      break;
    default:
      fprintf(stderr, "Usage: %s [-s scale] <rom>\n", argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  const char *rom_filename = argv[optind];

  ScreenT *screen =
      screen_new(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT, scale);

  Uxn *uxn = Uxn_new(screen);

  screen_boot(uxn);
  system_boot(uxn, (char *)rom_filename);

  bool continue_execution = true;

  Uxn_eval(uxn, RESET_VECTOR);

  while (continue_execution) {
    screen_update(uxn);

    handle_input(uxn);

    continue_execution = Uxn_dev_read(uxn, SYSTEM_STATE_PORT) == 0;
  }

  screen_delete(screen);
  Uxn_delete(uxn);

  return 0;
}
