#include "controller.h"

void controller_vector_eval(Uxn *uxn) {
  Short vector = Uxn_dev_read_short(uxn, CONTROLLER_VECTOR_PORT);
  Uxn_eval(uxn, vector);
}

void controller_button_down(Uxn *uxn, Byte mask) {
  if (mask) {
    Byte state = Uxn_dev_read(uxn, CONTROLLER_BUTTON_PORT) | mask;
    Uxn_dev_write(uxn, CONTROLLER_BUTTON_PORT, state);
    controller_vector_eval(uxn);
  }
}

void controller_button_up(Uxn *uxn, Byte mask) {
  if (mask) {
    Byte state = Uxn_dev_read(uxn, CONTROLLER_BUTTON_PORT) & (~mask);
    Uxn_dev_write(uxn, CONTROLLER_BUTTON_PORT, state);
    controller_vector_eval(uxn);
  }
}

void controller_key_down(Uxn *uxn, Byte key) {
  Uxn_dev_write(uxn, CONTROLLER_KEY_PORT, key);
  controller_vector_eval(uxn);
  Uxn_dev_write(uxn, CONTROLLER_KEY_PORT, 0);
}