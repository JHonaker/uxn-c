
#include "system.h"

#include "../common.h"
#include "../uxn.h"

#include <stdio.h>
#include <string.h>

// System expansion operations
static void system_expansion_fill(Uxn *uxn, Short op_addr) {
  Short length = uxn_mem_read_short(uxn, op_addr + 1);
  Short bank = uxn_mem_read_short(uxn, op_addr + 3);
  Short addr = uxn_mem_read_short(uxn, op_addr + 5);
  Byte value = uxn_mem_read(uxn, op_addr + 7);

  for (int i = 0; i < length; i++) {
    uxn_page_write(uxn, bank, addr + i, value);
  }
}

static void system_expansion_copy_left(Uxn *uxn, Short op_addr) {
  Short length = uxn_mem_read_short(uxn, op_addr + 1);
  Short src_bank = uxn_mem_read_short(uxn, op_addr + 3);
  Short src_addr = uxn_mem_read_short(uxn, op_addr + 5);
  Short dst_bank = uxn_mem_read_short(uxn, op_addr + 7);
  Short dst_addr = uxn_mem_read_short(uxn, op_addr + 9);

  for (int i = 0; i < length; i++) {
    Byte src = uxn_page_read(uxn, src_bank, src_addr + i);
    uxn_page_write(uxn, dst_bank, dst_addr + i, src);
  }
}

static void system_expansion_copy_right(Uxn *uxn, Short op_addr) {
  Short length = uxn_mem_read_short(uxn, op_addr + 1);
  Short src_bank = uxn_mem_read_short(uxn, op_addr + 3);
  Short src_addr = uxn_mem_read_short(uxn, op_addr + 5);
  Short dst_bank = uxn_mem_read_short(uxn, op_addr + 7);
  Short dst_addr = uxn_mem_read_short(uxn, op_addr + 9);

  for (int i = length - 1; i >= 0; i--) {
    Byte src = uxn_page_read(uxn, src_bank, src_addr + i);
    uxn_page_write(uxn, dst_bank, dst_addr + i, src);
  }
}

static void system_expansion(Uxn *uxn) {
  Short op_addr = uxn_dev_read_short(uxn, SYSTEM_EXPANSION_PORT);
  Byte operation = uxn_mem_read(uxn, op_addr);

  switch (operation) {
  case FILL:
    system_expansion_fill(uxn, op_addr);
    break;
  case CPYL:
    system_expansion_copy_left(uxn, op_addr);
    break;
  case CPYR:
    system_expansion_copy_right(uxn, op_addr);
    break;
  }
}

static int system_load(Uxn *uxn, char *filename) {
  FILE *f = fopen(filename, "rb");

  if (!f)
    return 0;

  Byte page_buffer[RAM_PAGE_SIZE] = {0};
  int page = 0;

  int l = fread(&page_buffer[RESET_VECTOR], RAM_PAGE_SIZE - RESET_VECTOR, 1, f);
  uxn_mem_load(uxn, page_buffer, sizeof(page_buffer), 0);

  while (l != 0 && ++page < RAM_PAGES) {
    printf("Loading page %d\n", page);
    memset(page_buffer, 0, sizeof(page_buffer));
    l = fread(page_buffer, RAM_PAGE_SIZE, 1, f);
    uxn_page_load(uxn, page_buffer, sizeof(page_buffer), page, 0);
  }

  fclose(f);

  return 1;
}

static void system_zero(Uxn *uxn, bool soft) {
  uxn_mem_zero(uxn, soft);
  uxn_dev_zero(uxn);
  uxn_stack_zero(uxn);
}

void system_inspect(Uxn *uxn) { uxn_dump(uxn); }

int system_error(char *msg, const char *err) {
  fprintf(stderr, "%s: %s\n", msg, err);
  fflush(stderr);
  return 0;
}

void system_reboot(Uxn *uxn, char *rom_path, bool soft) {
  system_zero(uxn, soft);
  system_load(uxn, rom_path);
  uxn_eval(uxn, RESET_VECTOR);
}

int system_boot(Uxn *uxn, char *rom_path) {
  if (!system_load(uxn, rom_path)) {
    return system_error("Error loading ROM", rom_path);
  }
  return 1;
}

Byte system_dei(Uxn *uxn, Byte addr) {

  switch (addr) {
  case SYSTEM_WST_PORT:
    return uxn_work_ptr(uxn);
  case SYSTEM_RST_PORT:
    return uxn_ret_ptr(uxn);
  default:
    return uxn_dev_read(uxn, addr);
  }
  return uxn_dev_read(uxn, addr);
}

void system_deo(Uxn *uxn, Byte addr) {
  switch (addr) {
  case SYSTEM_EXPANSION_PORT:
    system_expansion(uxn);
    break;
  case SYSTEM_WST_PORT:
    uxn_set_work_ptr(uxn, uxn_dev_read(uxn, SYSTEM_WST_PORT));
    break;
  case SYSTEM_RST_PORT:
    uxn_set_ret_ptr(uxn, uxn_dev_read(uxn, SYSTEM_RST_PORT));
    break;
  case SYSTEM_DEBUG_PORT:
    system_inspect(uxn);
    break;
  }
}