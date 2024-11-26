#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "device/console.h"
#include "device/raylib/controller.h"
#include "device/datetime.h"
#include "device/file.h"
#include "device/raylib/mouse.h"
#include "device/screen.h"
#include "device/system.h"
#include "uxn.h"




void handle_input(Uxn *uxn, int scale_factor) {
  console_poll(uxn);
  controller_poll(uxn);
  mouse_poll(uxn, scale_factor);
}

Byte uxn_dei_dispatch(Uxn *uxn, Byte addr) {
  const Byte page = addr & 0xf0;
  switch (page) {
  case DEVICE_PAGE_SYSTEM:
    return system_dei(uxn, addr);
  case DEVICE_PAGE_SCREEN:
    return screen_dei(uxn, addr);
  case DEVICE_PAGE_FILE1:
  case DEVICE_PAGE_FILE2:
    return file_dei(uxn, addr);
  case DEVICE_PAGE_DATETIME:
    return datetime_dei(uxn, addr);
  default:
    return uxn_dev_read(uxn, addr);
  }
}

void uxn_deo_dispatch(Uxn *uxn, Byte addr) {
  const Byte page = addr & 0xf0;
  switch (page) {
  case DEVICE_PAGE_SYSTEM: {
    system_deo(uxn, addr);
    if (SYSTEM_RED_PORT <= addr && addr <= SYSTEM_BLUE_PORT + 1) {
      screen_change_palette(uxn);
    }
    break;
  }
  case DEVICE_PAGE_CONSOLE:
    console_deo(uxn, addr);
    break;
  case DEVICE_PAGE_SCREEN:
    screen_deo(uxn, addr);
    break;
  case DEVICE_PAGE_FILE1:
  case DEVICE_PAGE_FILE2:
    file_deo(uxn, addr);
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

  SetExitKey(0);
  HideCursor();

  Uxn *uxn = uxn_new(screen);

  screen_boot(uxn);
  system_boot(uxn, (char *)rom_filename);

  bool continue_execution = true;

  uxn_eval(uxn, RESET_VECTOR);

  for (int i = optind + 1; i < argc; i++) {
    char *p = argv[i];
    while (*p) {
      console_input_event(uxn, *p++, CONSOLE_TYPE_ARG);
    }
    console_input_event(uxn, '\n',
                        i == argc - 1 ? CONSOLE_TYPE_ARG_END
                                      : CONSOLE_TYPE_ARG_SPACER);
  }

  while (continue_execution) {
    screen_update(uxn);

    handle_input(uxn, scale);

    continue_execution = uxn_dev_read(uxn, SYSTEM_STATE_PORT) == 0;
  }

  screen_delete(screen);
  uxn_delete(uxn);

  return 0;
}
