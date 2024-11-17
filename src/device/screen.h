#include "../common.h"
#include "../uxn.h"
#include <raylib.h>

#ifndef screen_h
#define screen_h

#define DEFAULT_SCREEN_WIDTH 512
#define DEFAULT_SCREEN_HEIGHT 320

#define SCREEN_VECTOR_PORT 0x20
#define SCREEN_WIDTH_PORT 0x22
#define SCREEN_HEIGHT_PORT 0x24
#define SCREEN_AUTO_PORT 0x26
#define SCREEN_X_PORT 0x28
#define SCREEN_Y_PORT 0x2a
#define SCREEN_ADDR_PORT 0x2c
#define SCREEN_PIXEL_PORT 0x2e
#define SCREEN_SPRITE_PORT 0x2f

#define SPRITE_WIDTH 8
#define SPRITE_HEIGHT 8
#define SPRITE_1BPP_BUFFER_SIZE 8
#define SPRITE_2BPP_BUFFER_SIZE 16

#define T RaylibScreen
#define ScreenT RaylibScreen

typedef struct T T;

typedef enum {
  PIXEL_MODE,
  FILL_MODE,
} PixelMode;

typedef enum { ONE_BIT, TWO_BIT } SpriteMode;

typedef enum { BG_LAYER, FG_LAYER } DrawLayer;

struct T {
  RenderTexture2D bg_buffer;
  RenderTexture2D fg_buffer;
  RenderTexture2D sprite_buffer;
  Short width;
  Short height;
  int scale;
  Color palette[4];
};

T *screen_new(int widht, int height, int scale);
void screen_delete(T *screen);

Byte screen_dei(Uxn *uxn, Byte addr);
void screen_deo(Uxn *uxn, Byte addr);

void screen_boot(Uxn *uxn);
void screen_update(Uxn *uxn);
void screen_change_palette(Uxn *uxn);

#undef T
#endif // screen_h