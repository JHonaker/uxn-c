#include "../common.h"
#include "../uxn.h"

#ifndef controller_h
#define controller_h

#define CONTROLLER_VECTOR_PORT 0x80
#define CONTROLLER_BUTTON_PORT 0x82
#define CONTROLLER_KEY_PORT 0x83

typedef enum {
  UXN_CONTROLLER_BUTTON_A = 0x01,
  UXN_CONTROLLER_BUTTON_B = 0x02,
  UXN_CONTROLLER_BUTTON_SELECT = 0x04,
  UXN_CONTROLLER_BUTTON_START = 0x08,
  UXN_CONTROLLER_BUTTON_UP = 0x10,
  UXN_CONTROLLER_BUTTON_DOWN = 0x20,
  UXN_CONTROLLER_BUTTON_LEFT = 0x40,
  UXN_CONTROLLER_BUTTON_RIGHT = 0x80,
} UxnControllerButton;

void controller_button_down(Uxn *uxn, Byte mask);
void controller_button_up(Uxn *uxn, Byte mask);
void controller_key_down(Uxn *uxn, Byte key);

#endif // controller_h