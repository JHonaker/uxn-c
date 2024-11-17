#include <raylib.h>
#include <rlgl.h>
#include <stdio.h>
#include <stdlib.h>

#include "screen.h"
#include "system.h"

#define T RaylibScreen

// clang-format off

// From the Varvara spec:
// c = !ch
//    ? (color % 5 ? color >> 2 : 0)
//    : color % 4 + ch == 1 ? 0 : (ch - 2 + (color & 3)) % 3 + 1;
static const Byte color_table[16][4] = {
    {0, 0, 1, 2}, // In 1bpp mode index 0 = clear and index 1 = transparent
    {0, 1, 2, 3}, 
    {0, 2, 3, 1},
    {0, 3, 1, 2},
    {1, 0, 1, 2},
    {0, 1, 2, 3}, // 0 in this mode is transparent
    {1, 2, 3, 1},
    {1, 3, 1, 2},
    {2, 0, 1, 2},
    {2, 1, 2, 3},
    {0, 2, 3, 1}, // 0 in this mode is transparent
    {2, 3, 1, 2},
    {3, 0, 1, 2},
    {3, 1, 2, 3},
    {3, 2, 3, 1},
    {0, 3, 1, 2}, // 0 in this mode is transparent
};

// clang-format on

static void set_transparent_mode(void) {
  rlSetBlendFactorsSeparate(RL_ONE, RL_ZERO, RL_ONE, RL_ZERO, RL_FUNC_ADD,
                            RL_FUNC_ADD);
}

static void set_clear_mode(void) {
  rlSetBlendFactorsSeparate(RL_ZERO, RL_ONE, RL_ONE, RL_ZERO, RL_FUNC_ADD,
                            RL_FUNC_ADD);
}

static void set_erase_with_alpha_mode(void) {
  rlSetBlendFactorsSeparate(RL_ZERO, RL_ONE_MINUS_SRC_ALPHA, RL_ZERO,
                            RL_ONE_MINUS_SRC_ALPHA, RL_FUNC_ADD, RL_FUNC_ADD);
}

// I want a mode that can be used to add pixels with RGB 255
// No Op for RGB 0
// And erase pixels with RGB 0

static Color pixel_color(Color palette[], Byte control) {
  Byte fg_layer = control & 0x40;
  Byte color = control & 0x03;

  return (color == 0 && fg_layer) ? BLANK : palette[color];
}
static void sprite_palette(Color palette[4], Color screen_colors[static 4],
                           DrawLayer layer, Byte color_byte) {

  // Load the colors from the screen palette
  for (size_t i = 0; i < 4; i++) {
    Byte color_index = color_table[color_byte][i];
    palette[i] = layer == FG_LAYER && color_index == 0
                     ? BLANK
                     : screen_colors[color_index];
  }

  if (color_byte % 5 == 0) {
    palette[0] = BLANK;
  }
}

void screen_init(T *screen, int width, int height, int scale) {
  InitWindow(width * scale, height * scale, "Uxn");

  *screen = (T){
      .bg_buffer = LoadRenderTexture(width, height),
      .fg_buffer = LoadRenderTexture(width, height),
      .sprite_buffer = LoadRenderTexture(SPRITE_WIDTH, SPRITE_HEIGHT),
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

  set_transparent_mode();
}

void screen_destroy(T *screen) {

  UnloadRenderTexture(screen->fg_buffer);
  UnloadRenderTexture(screen->bg_buffer);
  UnloadRenderTexture(screen->sprite_buffer);

  CloseWindow();
}

T *screen_new(int width, int height, int scale) {
  T *screen = (T *)malloc(sizeof(T));
  screen_init(screen, width, height, scale);
  return screen;
}

void screen_delete(T *screen) {
  screen_destroy(screen);
  free(screen);
}

static void draw_buffer(RenderTexture2D *buffer, int scale) {
  Texture2D texture = buffer->texture;

  DrawTexturePro(
      texture,
      // Source rect needs to flip Y axis
      (Rectangle){0, 0, (float)texture.width, (float)-texture.height},
      // Dest rect is scaled up
      (Rectangle){0, 0, (float)texture.width * scale,
                  (float)texture.height * scale},
      (Vector2){0, 0}, 0, WHITE);
}

void screen_redraw(Uxn *uxn, T *screen) {

  BeginDrawing();

  ClearBackground(screen->palette[0]);

  draw_buffer(&screen->bg_buffer, screen->scale);
  draw_buffer(&screen->fg_buffer, screen->scale);

  EndDrawing();

  if (WindowShouldClose())
    Uxn_dev_write(uxn, SYSTEM_STATE_PORT, 1);
}

void screen_boot(Uxn *uxn) {
  RaylibScreen *screen = Uxn_get_screen(uxn);

  Uxn_dev_write(uxn, SCREEN_WIDTH_PORT, screen->width >> 8);
  Uxn_dev_write(uxn, SCREEN_WIDTH_PORT + 1, screen->width & 0xff);

  Uxn_dev_write(uxn, SCREEN_HEIGHT_PORT, screen->height >> 8);
  Uxn_dev_write(uxn, SCREEN_HEIGHT_PORT + 1, screen->height & 0xff);
}



void screen_pixel_port(Uxn *uxn, T *screen) {
  Byte control = Uxn_dev_read(uxn, SCREEN_PIXEL_PORT);
  Byte color = control & 0x03;
  Byte fg_layer = control & 0x40;
  RenderTexture2D layer_texture =
      fg_layer ? screen->fg_buffer : screen->bg_buffer;
  Byte fill_mode = control & 0x80;

  Short x = Uxn_dev_read_short(uxn, SCREEN_X_PORT); 
  Short y = Uxn_dev_read_short(uxn, SCREEN_Y_PORT);

  Byte flip_x = control & 0x10;
  Byte flip_y = control & 0x20;

  Byte auto_byte = Uxn_dev_read(uxn, SCREEN_AUTO_PORT);
  Byte auto_x = auto_byte & 0x01;
  Byte auto_y = auto_byte & 0x02;

  Color draw_color = pixel_color(screen->palette, control);

  BeginTextureMode(layer_texture);

  if (fill_mode) {

    Byte rect_x = flip_x ? 0 : x;
    Byte rect_y = flip_y ? 0 : y;
    int rect_width = flip_x ? x : screen->width - x;
    int rect_height = flip_y ? y : screen->height - y;

    DrawRectangle(rect_x, rect_y, rect_width, rect_height, draw_color);

  } else {
    DrawPixel(x, y, screen->palette[color]);
  }

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

void read_sprite_chunk(Uxn *uxn, Byte buffer[SPRITE_BUFFER_SIZE], Short addr) {
  Uxn_mem_buffer_read(uxn, SPRITE_BUFFER_SIZE, buffer, addr);
}

void decode_1bpp_sprite(Byte sprite_data[SPRITE_BUFFER_SIZE],
                        Byte decoded_data[SPRITE_HEIGHT][SPRITE_WIDTH]) {
  for (int i = 0; i < SPRITE_HEIGHT; i++) {
    Byte row = sprite_data[i];
    Byte decoded_row = 0;
    for (int j = 0; j < SPRITE_WIDTH; j++) {
      Byte bit = (row >> j) & 0x01;
      decoded_data[i][7 - j] = bit;
    }
  }
}

void decode_2bpp_sprite(Byte low_bit_data[SPRITE_BUFFER_SIZE],
                        Byte high_bit_data[SPRITE_BUFFER_SIZE],
                        Byte decoded_data[SPRITE_HEIGHT][SPRITE_WIDTH]) {
  for (int i = 0; i < SPRITE_HEIGHT; i++) {
    Byte low_row = low_bit_data[i];
    Byte high_row = high_bit_data[i];
    for (int j = 0; j < SPRITE_WIDTH; j++) {
      Byte low_bit = (low_row >> j) & 0x01;
      Byte high_bit = (high_row >> j) & 0x01;
      decoded_data[i][7 - j] = (high_bit << 1) | low_bit;
    }
  }
}
void read_1bpp_sprite(Uxn *uxn, RenderTexture2D *buffer, Byte control,
                      Color palette[]) {
  Short addr = Uxn_dev_read_short(uxn, SCREEN_ADDR_PORT);

  Byte sprite_data[SPRITE_BUFFER_SIZE] = {0};
  read_sprite_chunk(uxn, sprite_data, addr);

  Byte decoded_data[SPRITE_HEIGHT][SPRITE_WIDTH] = {0};
  decode_1bpp_sprite(sprite_data, decoded_data);

  BeginTextureMode(*buffer);

  ClearBackground(BLANK);

  for (int j = 0; j < SPRITE_HEIGHT; j++) {
    for (int i = 0; i < SPRITE_WIDTH; i++) {
      Color color = palette[decoded_data[j][i]];
      DrawPixel(i, j, color);
    }
  }

  EndTextureMode();
}

void read_2bpp_sprite(Uxn *uxn, RenderTexture2D *buffer, Byte control,
                      Color palette[]) {
  Short addr = Uxn_dev_read_short(uxn, SCREEN_ADDR_PORT);

  Byte low_bit_data[SPRITE_BUFFER_SIZE] = {0};
  Byte high_bit_data[SPRITE_BUFFER_SIZE] = {0};
  read_sprite_chunk(uxn, low_bit_data, addr);
  read_sprite_chunk(uxn, high_bit_data, addr + SPRITE_BUFFER_SIZE);

  Byte decoded_data[SPRITE_HEIGHT][SPRITE_WIDTH] = {0};
  decode_2bpp_sprite(low_bit_data, high_bit_data, decoded_data);

  BeginTextureMode(*buffer);

  ClearBackground(BLANK);

  for (int j = 0; j < SPRITE_HEIGHT; j++) {
    for (int i = 0; i < SPRITE_WIDTH; i++) {
      Color color = palette[decoded_data[j][i]];
      DrawPixel(i, j, color);
    }
  }

  EndTextureMode();
}

static void shift_sprite_addr(Uxn *uxn, bool two_bit_mode) {
  Short addr = Uxn_dev_read_short(uxn, SCREEN_ADDR_PORT);
  addr += two_bit_mode ? 16 : 8;
  Uxn_dev_write_short(uxn, SCREEN_ADDR_PORT, addr);
}

void screen_draw_sprite(Uxn *uxn, T *screen, Byte control) {
  Short x = Uxn_dev_read_short(uxn, SCREEN_X_PORT);
  Short y = Uxn_dev_read_short(uxn, SCREEN_Y_PORT);

  Byte flip_x = control & 0x10;
  Byte flip_y = control & 0x20;
  DrawLayer layer = control & 0x40 ? FG_LAYER : BG_LAYER;
  Byte color = control & 0xf;

  Byte auto_byte = Uxn_dev_read(uxn, SCREEN_AUTO_PORT);
  Byte auto_x = auto_byte & 0x01;
  Byte auto_y = auto_byte & 0x02;

  Byte two_bit_mode = control & 0x80;

  Color palette[4] = {0};
  sprite_palette(palette, screen->palette, layer, color);

  bool erase_mode = color == 0 && !two_bit_mode;

  if (erase_mode) {
    palette[1] = WHITE;
    set_erase_with_alpha_mode();
  }

  int dirX = flip_x ? -1 : 1;
  int dirY = flip_y ? -1 : 1;

  Byte auto_addr = auto_byte & 0x04;
  Byte auto_length = (auto_byte & 0xf0) >> 4;

  Byte fg_layer = control & 0x40;
  RenderTexture2D layer_texture =
      fg_layer ? screen->fg_buffer : screen->bg_buffer;

  // Read sprite data
  if (two_bit_mode) {
    read_2bpp_sprite(uxn, &screen->sprite_buffer, control, palette);
  } else {
    read_1bpp_sprite(uxn, &screen->sprite_buffer, control, palette);
  }

  /**
   * The definition of dx and dy looks confusing
   * because of the test for auto_y in dx and vice versa.
   *
   * This is intended!
   *
   * According to the Varvara spec, extra sprites are drawn as columns moving
   * rightward for auto-x and as rows moving downward for auto-y
   */

  float dx = auto_y ? dirX * SPRITE_WIDTH : 0;
  float dy = auto_x ? dirY * SPRITE_HEIGHT : 0;

  for (int i = 0; i <= auto_length; i++) {
    BeginTextureMode(layer_texture);

    if (erase_mode) {
      BeginBlendMode(BLEND_CUSTOM_SEPARATE);
    }

    DrawTexturePro(
        screen->sprite_buffer.texture,
        (Rectangle){0, 0, dirX * SPRITE_WIDTH, dirY * -SPRITE_HEIGHT},
        (Rectangle){x + i * dx, y + i * dy, SPRITE_WIDTH, SPRITE_HEIGHT},
        (Vector2){0, 0}, 0, WHITE);

    if (erase_mode) {
      EndBlendMode();
    }

    EndTextureMode();

    if (auto_addr) {
      shift_sprite_addr(uxn, two_bit_mode);
      if (two_bit_mode) {
        read_2bpp_sprite(uxn, &screen->sprite_buffer, control, palette);
      } else {
        read_1bpp_sprite(uxn, &screen->sprite_buffer, control, palette);
      }
    }
  }

  if (auto_x) {
    x += dirX * SPRITE_WIDTH;
    Uxn_dev_write_short(uxn, SCREEN_X_PORT, x);
  }

  if (auto_y) {
    y += dirY * SPRITE_HEIGHT;
    Uxn_dev_write_short(uxn, SCREEN_Y_PORT, y);
  }
}

void screen_sprite_port(Uxn *uxn, T *screen) {
  screen_draw_sprite(uxn, screen, Uxn_dev_read(uxn, SCREEN_SPRITE_PORT));
}

void screen_change_palette(Uxn *uxn) {
  RaylibScreen *screen = Uxn_get_screen(uxn);


  Short red_bits = Uxn_dev_read_short(uxn, SYSTEM_RED_PORT);
  Short green_bits = Uxn_dev_read_short(uxn, SYSTEM_GREEN_PORT);
  Short blue_bits = Uxn_dev_read_short(uxn, SYSTEM_BLUE_PORT);

  for (int color = 0; color < 4; color++) {
    Byte shift = (3 - color) * 4;
    Byte red = (red_bits >> shift) & 0xf;
    Byte green = (green_bits >> shift) & 0xf;
    Byte blue = (blue_bits >> shift) & 0xf;

    // Convert from 4-bit to 8-bit
    red = red | (red << 4);
    green = green | (green << 4);
    blue = blue | (blue << 4);

    screen->palette[color] = (Color){
        .r = red,
        .g = green,
        .b = blue,
        .a = 255,
    };
  }
}

void screen_update(Uxn *uxn) {
  RaylibScreen *screen = Uxn_get_screen(uxn);
  Short screen_vector = Uxn_dev_read_short(uxn, SCREEN_VECTOR_PORT);

  Uxn_eval(uxn, screen_vector);
  screen_redraw(uxn, screen);
}

void screen_resize(Uxn *uxn) {
  RaylibScreen *screen = Uxn_get_screen(uxn);

  screen->width = Uxn_dev_read_short(uxn, SCREEN_WIDTH_PORT);
  screen->height = Uxn_dev_read_short(uxn, SCREEN_HEIGHT_PORT);

  SetWindowSize(screen->width * screen->scale, screen->height * screen->scale);
}

Byte screen_dei(Uxn *uxn, Byte addr) {
  RaylibScreen *screen = Uxn_get_screen(uxn);
  switch (addr) {
  case SCREEN_WIDTH_PORT:
    return screen->width >> 8;
  case SCREEN_WIDTH_PORT + 1:
    return screen->width & 0xff;
  case SCREEN_HEIGHT_PORT:
    return screen->height >> 8;
  case SCREEN_HEIGHT_PORT + 1:
    return screen->height & 0xff;
  default:
    return Uxn_dev_read(uxn, addr);
  }
}

void screen_deo(Uxn *uxn, Byte addr) {
  RaylibScreen *screen = Uxn_get_screen(uxn);

  switch (addr) {
  case SCREEN_WIDTH_PORT:
  case SCREEN_WIDTH_PORT + 1:
  case SCREEN_HEIGHT_PORT:
  case SCREEN_HEIGHT_PORT + 1:
    screen_resize(uxn);
    break;
  case SCREEN_PIXEL_PORT:
    screen_pixel_port(uxn, screen);
    break;
  case SCREEN_SPRITE_PORT:
    screen_sprite_port(uxn, screen);
    break;
  default:
    break;
  }
}