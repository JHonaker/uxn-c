#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "device/console.h"
#include "device/controller.h"
#include "device/datetime.h"
#include "device/file.h"
#include "device/mouse.h"
#include "device/screen.h"
#include "device/system.h"
#include "uxn.h"

void handle_mouse_button(Uxn *uxn, UxnMouseButton button,
                         MouseButton raylib_button) {
  if (IsMouseButtonPressed(raylib_button)) {
    mouse_button_down(uxn, button);
  }

  if (IsMouseButtonReleased(raylib_button)) {
    mouse_button_up(uxn, button);
  }
}
void handle_mouse(Uxn *uxn, int scale_factor) {
  Vector2 pos = GetMousePosition();
  mouse_move(uxn, pos.x / scale_factor, pos.y / scale_factor);

  handle_mouse_button(uxn, UXN_MOUSE_BUTTON_LEFT, MOUSE_LEFT_BUTTON);
  handle_mouse_button(uxn, UXN_MOUSE_BUTTON_MIDDLE, MOUSE_MIDDLE_BUTTON);
  handle_mouse_button(uxn, UXN_MOUSE_BUTTON_RIGHT, MOUSE_RIGHT_BUTTON);

  Vector2 scroll = GetMouseWheelMoveV();
  if (scroll.x != 0 || scroll.y != 0) {
    mouse_scroll(uxn, scroll.x, scroll.y);
  }
}
int convert_raylib_to_ascii(KeyboardKey key, bool shift_pressed) {
#define KDEF(k, shifted_k) (!shift_pressed ? k : shifted_k)

  // clang-format off

  switch (key) {
    case KEY_ESCAPE: return 0x1b;
    case KEY_GRAVE: return KDEF('`', '~');
    case KEY_ONE: return KDEF('1', '!');
    case KEY_TWO: return KDEF('2', '@');
    case KEY_THREE: return KDEF('3', '#');
    case KEY_FOUR: return KDEF('4', '$');
    case KEY_FIVE: return KDEF('5', '%');
    case KEY_SIX: return KDEF('6', '^');
    case KEY_SEVEN: return KDEF('7', '&');
    case KEY_EIGHT: return KDEF('8', '*');
    case KEY_NINE: return KDEF('9', '(');
    case KEY_ZERO: return KDEF('0', ')');
    case KEY_MINUS: return KDEF('-', '_');
    case KEY_EQUAL: return KDEF('=', '+');
    case KEY_BACKSPACE: return '\b';
    case KEY_TAB: return '\t';
    case KEY_Q: return KDEF('q', 'Q');
    case KEY_W: return KDEF('w', 'W');
    case KEY_E: return KDEF('e', 'E');
    case KEY_R: return KDEF('r', 'R');
    case KEY_T: return KDEF('t', 'T');
    case KEY_Y: return KDEF('y', 'Y');
    case KEY_U: return KDEF('u', 'U');
    case KEY_I: return KDEF('i', 'I');
    case KEY_O: return KDEF('o', 'O');
    case KEY_P: return KDEF('p', 'P');
    case KEY_LEFT_BRACKET: return KDEF('[', '{');
    case KEY_RIGHT_BRACKET: return KDEF(']', '}');
    case KEY_BACKSLASH: return KDEF('\\', '|');
    case KEY_A: return KDEF('a', 'A');
    case KEY_S: return KDEF('s', 'S');
    case KEY_D: return KDEF('d', 'D');
    case KEY_F: return KDEF('f', 'F');
    case KEY_G: return KDEF('g', 'G');
    case KEY_H: return KDEF('h', 'H');
    case KEY_J: return KDEF('j', 'J');
    case KEY_K: return KDEF('k', 'K');
    case KEY_L: return KDEF('l', 'L');
    case KEY_SEMICOLON: return KDEF(';', ':');
    case KEY_APOSTROPHE: return KDEF('\'', '"');
    case KEY_ENTER: return 0x0d;
    case KEY_Z: return KDEF('z', 'Z');
    case KEY_X: return KDEF('x', 'X');
    case KEY_C: return KDEF('c', 'C');
    case KEY_V: return KDEF('v', 'V');
    case KEY_B: return KDEF('b', 'B');
    case KEY_N: return KDEF('n', 'N');
    case KEY_M: return KDEF('m', 'M');
    case KEY_COMMA: return KDEF(',', '<');
    case KEY_PERIOD: return KDEF('.', '>');
    case KEY_SLASH: return KDEF('/', '?');
    case KEY_SPACE: return ' ';
    case KEY_INSERT: return 0x1b;
    case KEY_HOME: return 0x01;
    case KEY_PAGE_UP: return 0x02;
    case KEY_DELETE: return 0x7f;
    case KEY_END: return 0x05;
    case KEY_PAGE_DOWN: return 0x06;
    case KEY_KP_0: return '0';
    case KEY_KP_1: return '1';
    case KEY_KP_2: return '2';
    case KEY_KP_3: return '3';
    case KEY_KP_4: return '4';
    case KEY_KP_5: return '5';
    case KEY_KP_6: return '6';
    case KEY_KP_7: return '7';
    case KEY_KP_8: return '8';
    case KEY_KP_9: return '9';
    case KEY_KP_DECIMAL: return '.';
    case KEY_KP_DIVIDE: return '/';
    case KEY_KP_MULTIPLY: return '*';
    case KEY_KP_SUBTRACT: return '-';
    case KEY_KP_ADD: return '+';
    case KEY_KP_ENTER: return 0x0d;
    case KEY_KP_EQUAL: return '=';
    default: return 0;

    // clang-format on
  };
}

UxnControllerButton convert_raylib_button(KeyboardKey key) {
  switch (key) {
  case KEY_LEFT_CONTROL:
  case KEY_RIGHT_CONTROL:
    return UXN_CONTROLLER_BUTTON_A;
  case KEY_LEFT_ALT:
  case KEY_RIGHT_ALT:
    return UXN_CONTROLLER_BUTTON_B;
  case KEY_LEFT_SHIFT:
  case KEY_RIGHT_SHIFT:
    return UXN_CONTROLLER_BUTTON_SELECT;
  case KEY_HOME:
    return UXN_CONTROLLER_BUTTON_START;
  case KEY_UP:
    return UXN_CONTROLLER_BUTTON_UP;
  case KEY_DOWN:
    return UXN_CONTROLLER_BUTTON_DOWN;
  case KEY_LEFT:
    return UXN_CONTROLLER_BUTTON_LEFT;
  case KEY_RIGHT:
    return UXN_CONTROLLER_BUTTON_RIGHT;
  default:
    return 0x00;
  }
}

void handle_controller_button(Uxn *uxn, KeyboardKey raylib_key) {
  if (IsKeyPressed(raylib_key)) {
    controller_button_down(uxn, convert_raylib_button(raylib_key));
  } else if (IsKeyReleased(raylib_key)) {
    controller_button_up(uxn, convert_raylib_button(raylib_key));
  }
}
void handle_keyboard(Uxn *uxn) {

  handle_controller_button(uxn, KEY_LEFT_CONTROL);
  handle_controller_button(uxn, KEY_RIGHT_CONTROL);
  handle_controller_button(uxn, KEY_LEFT_ALT);
  handle_controller_button(uxn, KEY_RIGHT_ALT);
  handle_controller_button(uxn, KEY_LEFT_SHIFT);
  handle_controller_button(uxn, KEY_RIGHT_SHIFT);
  handle_controller_button(uxn, KEY_HOME);
  handle_controller_button(uxn, KEY_UP);
  handle_controller_button(uxn, KEY_DOWN);
  handle_controller_button(uxn, KEY_LEFT);
  handle_controller_button(uxn, KEY_RIGHT);

  KeyboardKey key = 0;
  while ((key = GetKeyPressed())) {
    bool shift_pressed =
        IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
    controller_key_down(uxn, convert_raylib_to_ascii(key, shift_pressed));
  }
}

void handle_input(Uxn *uxn, int scale_factor) {
  console_poll(uxn);
  handle_keyboard(uxn);
  handle_mouse(uxn, scale_factor);
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
