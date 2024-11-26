#include "../mouse.h"
#include "../../common.h"
#include <raylib.h>

void handle_mouse_button(Uxn *uxn, UxnMouseButton button,
                         MouseButton raylib_button) {
  if (IsMouseButtonPressed(raylib_button)) {
    mouse_button_down(uxn, button);
  }

  if (IsMouseButtonReleased(raylib_button)) {
    mouse_button_up(uxn, button);
  }
}

void mouse_poll(Uxn *uxn, int scale_factor) {
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