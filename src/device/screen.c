#include <raylib.h>

#include "system.h"
#include "screen.h"
#include "stdlib.h"

#define T RaylibScreen

void screen_init(T* screen, int width, int height, int scale) {
  InitWindow(width * scale, height * scale, "Uxn");

  *screen = (T) {
    .background = (Byte*) malloc(width * height),
    .foreground = (Byte*) malloc(width * height),
    .pixel_buffer = LoadRenderTexture(width, height),
    .width = width,
    .height = height,
    .scale = scale,
    .palette = {0},
  };

  SetTargetFPS(60);

  BeginTextureMode(screen->pixel_buffer);
  ClearBackground(RAYWHITE);
  EndTextureMode();
}

void screen_destroy(T* screen) {
  
  free(screen->background);
  free(screen->foreground);
  UnloadRenderTexture(screen->pixel_buffer);
  
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

void screen_redraw(Uxn* uxn, T* screen) {

  BeginDrawing();

  ClearBackground(RAYWHITE);
  
  Texture2D texture = screen->pixel_buffer.texture;

  DrawTexturePro(
    texture, 
    (Rectangle) {0, 0, 
      (float) texture.width,
      (float) -texture.height},
      (Rectangle) {0, 0, 
        (float) texture.width * screen->scale,
        (float) texture.height * screen->scale},
      (Vector2) {0, 0},
      0,
      WHITE);

  EndDrawing();

  if (WindowShouldClose()) Uxn_dev_write(uxn, SYSTEM_STATE_PORT, 1);
}

Byte screen_dei(Uxn* uxn, T* screen, Byte addr) {
  switch (addr) {
    case SCREEN_WIDTH_PORT:       return screen->width >> 8;
    case SCREEN_WIDTH_PORT + 1:   return screen->width & 0xff;
    case SCREEN_HEIGHT_PORT:      return screen->height >> 8;
    case SCREEN_HEIGHT_PORT + 1:  return screen->height & 0xff;
    default: return Uxn_dev_read(uxn, addr);
  }
}