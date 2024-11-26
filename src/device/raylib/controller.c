#include "controller.h"
#include "../controller.h"
#include <raylib.h>

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
void controller_poll(Uxn *uxn) {

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