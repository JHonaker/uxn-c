#include "../common.h"
#include "../uxn.h"

#define MOUSE_VECTOR_PORT 0x90
#define MOUSE_X_PORT 0x92
#define MOUSE_Y_PORT 0x94
#define MOUSE_STATE_PORT 0x96
#define MOUSE_SCROLLX_PORT 0x9a
#define MOUSE_SCROLLY_PORT 0x9c

#define UXN_MOUSE_SCROLL_POSITIVE 0x01
#define UXN_MOUSE_SCROLL_NEGATIVE 0xffff

typedef enum {
  UXN_MOUSE_BUTTON_LEFT = 0x01,
  UXN_MOUSE_BUTTON_MIDDLE = 0x02,
  UXN_MOUSE_BUTTON_RIGHT = 0x04,
} UxnMouseButton;

void mouse_button_down(Uxn *uxn, Byte mask);
void mouse_button_up(Uxn *uxn, Byte mask);
void mouse_move(Uxn *uxn, Short x, Short y);
void mouse_scroll(Uxn *uxn, Short x, Short y);