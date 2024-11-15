#include <raylib.h>
#include <rlgl.h>

#include "system.h"
#include "screen.h"
#include "stdlib.h"

#define T RaylibScreen

void screen_init(T* screen, int width, int height, int scale) {
  InitWindow(width * scale, height * scale, "Uxn");

  *screen = (T) {
    .bg_buffer = LoadRenderTexture(width, height),
    .fg_buffer = LoadRenderTexture(width, height),
    .sprite_buffer = {0},
    .width = width,
    .height = height,
    .scale = scale,
    .palette = {RED, GREEN, BLUE, MAGENTA},
  };

  SetTargetFPS(60);

  BeginTextureMode(screen->fg_buffer);
  ClearBackground(BLANK);
  EndTextureMode();
  BeginTextureMode(screen->bg_buffer);
  ClearBackground(BLANK);
  EndTextureMode();

  rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
}

void screen_destroy(T* screen) {
  
  UnloadRenderTexture(screen->fg_buffer);
  UnloadRenderTexture(screen->bg_buffer);
  
  CloseWindow();
}

T* screen_new(int width, int height, int scale) {
  T* screen = (T*) malloc(sizeof(T));
  screen_init(screen, width, height, scale);
  return screen;
}

void screen_delete(T* screen) {
  screen_destroy(screen);
  free(screen);
}

static void draw_buffer(RenderTexture2D* buffer, int scale) {
  Texture2D texture = buffer->texture;

  DrawTexturePro(
    texture, 
    // Source rect needs to flip Y axis
    (Rectangle) {0, 0, 
      (float) texture.width,
      (float) -texture.height},
    // Dest rect is scaled up
    (Rectangle) {0, 0, 
      (float) texture.width * scale,
      (float) texture.height * scale},
    (Vector2) {0, 0},
    0,
    WHITE);
}

void screen_redraw(Uxn* uxn, T* screen) {

  BeginDrawing();

  ClearBackground(RAYWHITE);
  
  draw_buffer(&screen->bg_buffer, screen->scale);
  draw_buffer(&screen->fg_buffer, screen->scale);

  EndDrawing();

  if (WindowShouldClose()) Uxn_dev_write(uxn, SYSTEM_STATE_PORT, 1);
}

void screen_boot(Uxn* uxn) {
  RaylibScreen* screen = Uxn_get_screen(uxn);

  Uxn_dev_write(uxn, SCREEN_WIDTH_PORT, screen->width >> 8);
  Uxn_dev_write(uxn, SCREEN_WIDTH_PORT + 1, screen->width & 0xff);

  Uxn_dev_write(uxn, SCREEN_HEIGHT_PORT, screen->height >> 8);
  Uxn_dev_write(uxn, SCREEN_HEIGHT_PORT + 1, screen->height & 0xff);
}

void screen_pixel_port(Uxn* uxn, T* screen) {
  Byte control = Uxn_dev_read(uxn, SCREEN_PIXEL_PORT);
  Byte color = control & 0x03;
  Byte fg_layer = control & 0x40;
  RenderTexture2D layer_texture = fg_layer ? screen->fg_buffer : screen->bg_buffer;
  Byte fill_mode = control & 0x80;

  Byte high_x = Uxn_dev_read(uxn, SCREEN_X_PORT);
  Byte low_x = Uxn_dev_read(uxn, SCREEN_X_PORT + 1);
  Short x = high_x << 8 | low_x;

  Byte high_y = Uxn_dev_read(uxn, SCREEN_Y_PORT);
  Byte low_y = Uxn_dev_read(uxn, SCREEN_Y_PORT + 1);
  Short y = high_y << 8 | low_y;

  Byte flip_x = control & 0x10;
  Byte flip_y = control & 0x20;

  Byte auto_byte = Uxn_dev_read(uxn, SCREEN_AUTO_PORT);
  Byte auto_x = auto_byte & 0x01;
  Byte auto_y = auto_byte & 0x02;

  Color draw_color = (color == 0 && fg_layer) ? BLANK : screen->palette[color];
  
  BeginTextureMode(layer_texture);
  BeginBlendMode(BLEND_CUSTOM);

  if (fill_mode) {

    Byte rect_x = flip_x ? 0 : x;
    Byte rect_y = flip_y ? 0 : y;
    int rect_width = flip_x ? x : screen->width - x;
    int rect_height = flip_y ? y : screen->height - y;

    DrawRectangle(rect_x, rect_y, rect_width, rect_height, draw_color);

  } else {
    DrawPixel(x, y, screen->palette[color]);
  }
  
  EndBlendMode();
  EndTextureMode();

  if (auto_x) {
    x++;
    Uxn_dev_write(uxn, SCREEN_X_PORT, x >> 8);
    Uxn_dev_write(uxn, SCREEN_X_PORT + 1, x & 0xff);
  }

  if (auto_y) {
    y++;
    Uxn_dev_write(uxn, SCREEN_Y_PORT, y >> 8);
    Uxn_dev_write(uxn, SCREEN_Y_PORT + 1, y & 0xff);
  }
}

void screen_change_palette(Uxn* uxn) {
  RaylibScreen* screen = Uxn_get_screen(uxn);

  Byte high_red = Uxn_dev_read(uxn, SYSTEM_RED_PORT);
  Byte low_red = Uxn_dev_read(uxn, SYSTEM_RED_PORT + 1);
  Short red_bits = high_red << 8 | low_red;

  Byte high_green = Uxn_dev_read(uxn, SYSTEM_GREEN_PORT);
  Byte low_green = Uxn_dev_read(uxn, SYSTEM_GREEN_PORT + 1);
  Short green_bits = high_green << 8 | low_green;

  Byte high_blue = Uxn_dev_read(uxn, SYSTEM_BLUE_PORT);
  Byte low_blue = Uxn_dev_read(uxn, SYSTEM_BLUE_PORT + 1);
  Short blue_bits = high_blue << 8 | low_blue;

  for (int color = 0; color < 4; color++) {
    Byte shift = (3 - color) * 4;
    Byte red = (red_bits >> shift) & 0xf;
    Byte green = (green_bits >> shift) & 0xf;
    Byte blue = (blue_bits >> shift) & 0xf;

    // Convert from 4-bit to 8-bit
    red = red | (red << 4);
    green = green | (green << 4);
    blue = blue | (blue << 4);

    screen->palette[color] = (Color) {
      .r = red,
      .g = green,
      .b = blue,
      .a = 255,
    };
  }
}

void screen_update(Uxn* uxn) {
  RaylibScreen* screen = Uxn_get_screen(uxn);
  Byte high = Uxn_dev_read(uxn, SCREEN_VECTOR_PORT);
  Byte low = Uxn_dev_read(uxn, SCREEN_VECTOR_PORT + 1);
  Short screen_vector = high << 8 | low;
  
  Uxn_eval(uxn, screen_vector);
  screen_redraw(uxn, screen);
}

void screen_resize(Uxn* uxn) {
  RaylibScreen* screen = Uxn_get_screen(uxn);

  Byte high_width = Uxn_dev_read(uxn, SCREEN_WIDTH_PORT);
  Byte low_width = Uxn_dev_read(uxn, SCREEN_WIDTH_PORT + 1);
  screen->width = high_width << 8 | low_width;

  Byte high_height = Uxn_dev_read(uxn, SCREEN_HEIGHT_PORT);
  Byte low_height = Uxn_dev_read(uxn, SCREEN_HEIGHT_PORT + 1);
  screen->height = high_height << 8 | low_height;

  SetWindowSize(screen->width * screen->scale, screen->height * screen->scale);
}

Byte screen_dei(Uxn* uxn, Byte addr) {
  RaylibScreen* screen = Uxn_get_screen(uxn);
  switch (addr) {
    case SCREEN_WIDTH_PORT:       return screen->width >> 8;
    case SCREEN_WIDTH_PORT + 1:   return screen->width & 0xff;
    case SCREEN_HEIGHT_PORT:      return screen->height >> 8;
    case SCREEN_HEIGHT_PORT + 1:  return screen->height & 0xff;
    default: return Uxn_dev_read(uxn, addr);
  }
}

void screen_deo(Uxn* uxn, Byte addr) {
  RaylibScreen* screen = Uxn_get_screen(uxn);

  switch (addr) {
    case SCREEN_WIDTH_PORT:
    case SCREEN_WIDTH_PORT + 1:
    case SCREEN_HEIGHT_PORT:
    case SCREEN_HEIGHT_PORT + 1: screen_resize(uxn); break;
    case SCREEN_PIXEL_PORT: 
      screen_pixel_port(uxn, screen);
      break;
    default: break;
  }
}