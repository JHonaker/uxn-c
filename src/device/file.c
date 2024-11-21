#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_PAGE(addr) ((addr) & 0xf0)
#define FILE_IDX(addr) (((addr) & 0xa0) == 0xa0 ? FILE_A : FILE_B)

#define MAX_FILE_NAME_LENGTH 0xff
#define FILE_BUFFER_SIZE 0xffff

typedef enum { FILE_A, FILE_B, FILE_COUNT } UxnFileIndex;

typedef enum { UXN_FT_FILE, UXN_FT_DIR, UXN_FT_COUNT } UxnFileType;

typedef enum {
  STATE_INIT,
  STATE_READ,
  STATE_WRITE,
  STATE_APPEND,
  STATE_CLOSED,
  STATE_COUNT
} UxnFileState;

struct UxnFile {
  char *name;
  FILE *fp;
  UxnFileType type;
  UxnFileState state;
};

void file_init(struct UxnFile *file, char *filename) {
  *file = (struct UxnFile){
      .fp = NULL, .name = filename, .type = UXN_FT_FILE, .state = STATE_INIT};
}

int file_open(struct UxnFile *file, UxnFileState state) {
  char *mode = NULL;

  switch (state) {
  case STATE_READ:
    mode = "r";
    break;
  case STATE_WRITE:
    mode = "w";
    break;
  case STATE_APPEND:
    mode = "a";
    break;
  default:
    return 0;
  }

  file->fp = fopen(file->name, mode);
  file->state = state;

  return file->fp ? 1 : 0;
}

int file_close(struct UxnFile *file) {
  if (file->fp) {
    fclose(file->fp);
    file->fp = NULL;
  }

  if (file->name) {
    free(file->name);
    file->name = NULL;
  }

  file->state = STATE_CLOSED;

  return 1;
}

int file_reopen(struct UxnFile *file, UxnFileState state) {
  fclose(file->fp);
  file->fp = NULL;
  return file_open(file, state);
}
void file_name_port_deo(Uxn *uxn, struct UxnFile *file, Byte page) {

  file_close(file);

  Short name_addr = Uxn_dev_read_short(uxn, page | FILE_NAME_PORT);
  Byte buffer[MAX_FILE_NAME_LENGTH] = {0};
  Uxn_mem_buffer_read(uxn, MAX_FILE_NAME_LENGTH, buffer, name_addr);
  buffer[MAX_FILE_NAME_LENGTH - 1] = '\0';

  size_t name_len = strlen((char *)buffer);
  char *name = calloc(name_len + 1, sizeof(char));
  strcpy(name, (char *)buffer);

  file_init(file, name);
}

void file_read(Uxn *uxn, struct UxnFile *file, Byte page) {
  if (!file->fp) {
    Uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, 0);
    return;
  }

  Byte buffer[FILE_BUFFER_SIZE] = {0};
  Short bytes_to_read = Uxn_dev_read_short(uxn, page | FILE_LENGTH_PORT);
  Short bytes_read = fread(buffer, 1, bytes_to_read, file->fp);

  Uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, bytes_read);

  if (!bytes_read)
    return;

  Short write_addr = Uxn_dev_read_short(uxn, page | FILE_READ_PORT);
  Uxn_mem_load(uxn, buffer, bytes_read, write_addr);
}

void file_read_port_deo(Uxn *uxn, struct UxnFile *file, Byte page) {
  switch (file->state) {
  case STATE_INIT:
    if (file_open(file, STATE_READ))
      file_read(uxn, file, page);
    break;
  case STATE_READ:
    file_read(uxn, file, page);
    break;
  case STATE_WRITE:
    if (file_reopen(file, STATE_READ))
      file_read(uxn, file, page);
    break;
  case STATE_APPEND:
    if (file_reopen(file, STATE_READ))
      file_read(uxn, file, page);
    break;
  case STATE_CLOSED:
    file_open(file, STATE_READ);
    file_read(uxn, file, page);
  default:
    break;
  }
}

void file_write(Uxn *uxn, struct UxnFile *file, Byte page) {
  if (!file->fp) {
    Uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, 0);
    return;
  }

  Short mem_addr = Uxn_dev_read_short(uxn, page | FILE_WRITE_PORT);
  Short bytes_to_write = Uxn_dev_read_short(uxn, page | FILE_LENGTH_PORT);
  Byte buffer[FILE_BUFFER_SIZE] = {0};

  Uxn_mem_buffer_read(uxn, bytes_to_write, buffer, mem_addr);
  Short bytes_written = fwrite(buffer, 1, bytes_to_write, file->fp);

  Uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, bytes_written);
}

void file_write_port_deo(Uxn *uxn, struct UxnFile *file, Byte page) {
  Short append_mode = Uxn_dev_read(uxn, page | FILE_APPEND_PORT);
  UxnFileState state = append_mode ? STATE_APPEND : STATE_WRITE;

  switch (file->state) {
  case STATE_INIT && !append_mode:
    if (file_open(file, state))
      file_write(uxn, file, page);
    break;
  case STATE_READ:
    if (file_reopen(file, state))
      file_write(uxn, file, page);
    break;
  case STATE_WRITE:
  case STATE_APPEND:
    if (file->state != state) {
      if (file_reopen(file, state))
        file_write(uxn, file, page);
    } else {
      file_write(uxn, file, page);
    }
    break;
  case STATE_CLOSED:
    file_open(file, state);
    file_write(uxn, file, page);
  default:
    break;
  }
}

void file_delete_port_deo(Uxn *uxn, struct UxnFile *file, Byte page) {
  remove(file->name);
  file_close(file);
}

void file_deo(Uxn *uxn, Byte addr) {
  struct UxnFile *files = Uxn_get_open_files(uxn);

  if (!files) {
    files = calloc(FILE_COUNT, sizeof(struct UxnFile));

    if (files) {
      files[FILE_A] = (struct UxnFile){0};
      files[FILE_B] = (struct UxnFile){0};
      Uxn_set_open_files(uxn, files);
    } else {
      Uxn_set_open_files(uxn, NULL);
      printf("Failed to allocate memory for files\n");
    }
  }

  struct UxnFile *file = &files[FILE_IDX(addr)];

  // clang-format off
  switch (addr & 0x0f) {
    case FILE_NAME_PORT: file_name_port_deo(uxn, file, FILE_PAGE(addr)); break;
    case FILE_READ_PORT: file_read_port_deo(uxn, file, FILE_PAGE(addr)); break;
    case FILE_WRITE_PORT: file_write_port_deo(uxn, file, FILE_PAGE(addr)); break;
    case FILE_DELETE_PORT: file_delete_port_deo(uxn, file, FILE_PAGE(addr)); break;
  }
  // clang-format on
}

Byte file_dei(Uxn *uxn, Byte addr) {
  // clang-format off
  switch (addr & 0x0f) {
    // case FILE_VECTOR_PORT: break;
    // case FILE_SUCCESS_PORT: break;
    // case FILE_STAT_PORT: break;
    // case FILE_DELETE_PORT: break;
    // case FILE_APPEND_PORT: break;
    // case FILE_NAME_PORT: break;
    // case FILE_LENGTH_PORT: break;
    // case FILE_READ_PORT: break;
    // case FILE_WRITE_PORT: break;
    default: return Uxn_dev_read(uxn, addr);
  }
  // clang-format on
}