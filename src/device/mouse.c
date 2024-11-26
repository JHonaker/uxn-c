#include "mouse.h"
#include "../common.h"
#include "../uxn.h"

void mouse_vector_eval(Uxn *uxn) {
  Short vector = uxn_dev_read_short(uxn, MOUSE_VECTOR_PORT);
  uxn_eval(uxn, vector);
}

void mouse_button_down(Uxn *uxn, Byte mask) {
  Byte state = uxn_dev_read(uxn, MOUSE_STATE_PORT) | mask;
  uxn_dev_write(uxn, MOUSE_STATE_PORT, state);
  mouse_vector_eval(uxn);
}

void mouse_button_up(Uxn *uxn, Byte mask) {
  Byte state = uxn_dev_read(uxn, MOUSE_STATE_PORT) & (~mask);
  uxn_dev_write(uxn, MOUSE_STATE_PORT, state);
  mouse_vector_eval(uxn);
}

void mouse_move(Uxn *uxn, Short x, Short y) {
  uxn_dev_write_short(uxn, MOUSE_X_PORT, x);
  uxn_dev_write_short(uxn, MOUSE_Y_PORT, y);
  
  mouse_vector_eval(uxn);
}

void mouse_scroll(Uxn *uxn, Short x, Short y) {
  uxn_dev_write_short(uxn, MOUSE_SCROLLX_PORT, x);
  uxn_dev_write_short(uxn, MOUSE_SCROLLY_PORT, -y);

  mouse_vector_eval(uxn);

  uxn_dev_write_short(uxn, MOUSE_SCROLLX_PORT, 0);
  uxn_dev_write_short(uxn, MOUSE_SCROLLY_PORT, 0);
}
