#include <raylib.h>
#include "../common.h"
#include "../uxn.h"

#ifndef screen_h
#define screen_h

#define SCREEN_VECTOR_PORT 0x20
#define SCREEN_WIDTH_PORT 0x22
#define SCREEN_HEIGHT_PORT 0x24
#define SCREEN_AUTO_PORT 0x26
#define SCREEN_X_PORT 0x28
#define SCREEN_Y_PORT 0x2a
#define SCREEN_ADDR_PORT 0x2c
#define SCREEN_PIXEL_PORT 0x2e
#define SCREEN_SPRITE_PORT 0x2f

#define T RaylibScreen

typedef struct T T;


typedef enum {
  PIXEL_MODE,
  FILL_MODE,
} PixelMode;

typedef enum {
  ONE_BIT,
  TWO_BIT
} SpriteMode;

typedef enum {
  BG_LAYER,
  FG_LAYER
} DrawLayer;


struct T {
  Byte* background;
  Byte* foreground;
  RenderTexture2D pixel_buffer;
  Short width;
  Short height;
  int scale;
  uint32_t palette[4];
};

T* screen_new(int widht, int height, int scale);
void screen_delete(T* screen);

Byte screen_dei(Uxn* uxn, T* screen, Byte addr);
void screen_deo(Uxn* uxn, T* screen, Byte addr);

void screen_redraw(Uxn* uxn, T* screen);

#undef T
#endif // screen_h