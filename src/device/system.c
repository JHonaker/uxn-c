
#include "system.h"

#include "../common.h"
#include "../uxn.h"

#include <stdio.h>

// System expansion operations
static void system_expansion_fill(Uxn* uxn) {
  Short length = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 1);
  Short bank = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 3);
  Short addr = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 5);
  Byte value = Uxn_mem_read(uxn, SYSTEM_EXPANSION_PORT + 7);
  
  for (int i = 0; i < length; i++) {
    Uxn_page_write(uxn, bank, addr + i, value);
  }
}

static void system_expansion_copy_left(Uxn* uxn) {
  Short length = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 1);
  Short src_bank = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 3);
  Short src_addr = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 5);
  Short dst_bank = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 7);
  Short dst_addr = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 9);

  for (int i = 0; i < length; i++) {
    Byte src = Uxn_page_read(uxn, src_bank, src_addr + i);
    Uxn_page_write(uxn, dst_bank, dst_addr + i, src);
  }
}

static void system_expansion_copy_right(Uxn* uxn) {
  Short length = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 1);
  Short src_bank = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 3);
  Short src_addr = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 5);
  Short dst_bank = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 7);
  Short dst_addr = Uxn_mem_read_short(uxn, SYSTEM_EXPANSION_PORT + 9);

  for (int i = length - 1; i >= 0; i--) {
    Byte src = Uxn_page_read(uxn, src_bank, src_addr + i);
    Uxn_page_write(uxn, dst_bank, dst_addr + i, src);
  }
}

static void system_expansion(Uxn* uxn) {
  Byte operation = Uxn_dev_read(uxn, SYSTEM_EXPANSION_PORT);

  switch (operation) {
    case FILL: system_expansion_fill(uxn); break;
    case CPYL: system_expansion_copy_left(uxn); break;
    case CPYR: system_expansion_copy_right(uxn); break;
  }
}

static int system_load(Uxn* uxn, char* filename) {
  FILE* f = fopen(filename, "rb");

  if (!f) return 0;

  Byte page_buffer[RAM_PAGE_SIZE] = {0};
  int page = 0;

  int l = fread(&page_buffer[RESET_VECTOR], RAM_PAGE_SIZE - RESET_VECTOR, 1, f);
  Uxn_mem_load(uxn, page_buffer, sizeof(page_buffer), 0);

 
  while (l != 0 && ++page < RAM_PAGES) {
    l = fread(page_buffer, RAM_PAGE_SIZE, 1, f);
    Uxn_mem_load(uxn, page_buffer, sizeof(page_buffer), page * RAM_PAGE_SIZE);
  }

  fclose(f);

  return 1;
}

static void system_zero(Uxn* uxn, bool soft) {
  Uxn_mem_zero(uxn, soft);
  Uxn_dev_zero(uxn);
  Uxn_stack_zero(uxn);
}

void system_inspect(Uxn* uxn) {
  Uxn_dump(uxn);
}

int system_error(char* msg, const char* err) {
  fprintf(stderr, "%s: %s\n", msg, err);
  fflush(stderr);
  return 0;
}

void system_reboot(Uxn* uxn, char* rom_path, bool soft) {
  system_zero(uxn, soft);
  system_load(uxn, rom_path);
  Uxn_eval(uxn, RESET_VECTOR);
}

int system_boot(Uxn* uxn, char* rom_path) {
  if (!system_load(uxn, rom_path)) {
    return system_error("Error loading ROM", rom_path);
  }
  return 1;
}

Byte system_dei(Uxn* uxn, Byte addr) {

  switch (addr) {
    case 0x04: return Uxn_work_ptr(uxn);
    case 0x05: return Uxn_ret_ptr(uxn);
    default: return Uxn_dev_read(uxn, addr);
  }
  return Uxn_dev_read(uxn, addr);
}

void system_deo(Uxn* uxn, Byte addr) {
  switch (addr) {
    case SYSTEM_EXPANSION_PORT: system_expansion(uxn); break;
    case SYSTEM_WST_PORT: Uxn_set_work_ptr(uxn, Uxn_dev_read(uxn, 0x04)); break;
    case SYSTEM_RST_PORT: Uxn_set_ret_ptr(uxn, Uxn_dev_read(uxn, 0x05)); break;
    case SYSTEM_DEBUG_PORT: system_inspect(uxn); break;
  }
}