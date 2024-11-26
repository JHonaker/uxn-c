#include "file.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define ENSURE_BUFFER_BOUNDS(addr, len)                                        \
  if (addr + len > RAM_PAGE_SIZE) {                                            \
    len = RAM_PAGE_SIZE - addr;                                                \
  }
#define FILE_PAGE(addr) ((addr) & 0xf0)
#define FILE_IDX(addr) (((addr) & 0xa0) == 0xa0 ? FILE_A : FILE_B)

#define MAX_FILE_NAME_LENGTH 0xff
#define FILE_BUFFER_SIZE 0xffff
#define FILE_COUNT 2

typedef enum { FILE_A, FILE_B } UxnFileIndex;

typedef enum { UXN_FILE_TYPE, UXN_DIR_TYPE } UxnStreamType;

typedef enum {
  STATE_INIT,
  STATE_READ,
  STATE_WRITE,
  STATE_APPEND,
  STATE_CLOSED,
  STATE_COUNT
} UxnFileState;

typedef struct UxnFile {
  UxnStreamType type;
  char *name;
  UxnFileState state;
  FILE *fp;
} UxnFile;

typedef struct UxnDir {
  UxnStreamType type;
  char *name;
  UxnFileState state;
  DIR *dp;
  char *content;
  size_t read_offset;
} UxnDir;

typedef union UxnStream {
  struct UxnFile file;
  struct UxnDir dir;
} UxnStream;

int stream_init(UxnStream *stream, char *name);
void stream_close(UxnStream *stream);

char *read_filename(Uxn *uxn, Short addr) {
  char *name = calloc(MAX_FILE_NAME_LENGTH + 1, sizeof(Byte));
  Short bytes_to_read = MAX_FILE_NAME_LENGTH;

  ENSURE_BUFFER_BOUNDS(addr, bytes_to_read);

  uxn_mem_buffer_read(uxn, bytes_to_read, (Byte *)name, addr);
  name[bytes_to_read] = '\0';

  size_t name_len = strlen((char *)name);
  char *new_buf = realloc(name, (name_len + 1) * sizeof(char));

  if (new_buf) {
    name = new_buf;
  }

  return name;
}

void file_init(UxnFile *file, char *filename) {
  *file = (struct UxnFile){
      .fp = NULL, .name = filename, .type = UXN_FILE_TYPE, .state = STATE_INIT};
}

int file_open(UxnFile *file, UxnFileState state) {
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

void file_close(UxnFile *file) {
  if (file->fp) {
    fclose(file->fp);
    file->fp = NULL;
  }

  if (file->name) {
    free(file->name);
    file->name = NULL;
  }
}

int file_reopen(UxnFile *file, UxnFileState state) {
  if (file->fp) {
    fclose(file->fp);
    file->fp = NULL;
  }
  return file_open(file, state);
}

void file_read(Uxn *uxn, UxnFile *file, Byte page) {
  if (!file->fp) {
    uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, 0);
    return;
  }

  Byte buffer[FILE_BUFFER_SIZE] = {0};
  Short bytes_to_read = uxn_dev_read_short(uxn, page | FILE_LENGTH_PORT);
  bytes_to_read =
      (bytes_to_read > FILE_BUFFER_SIZE) ? FILE_BUFFER_SIZE : bytes_to_read;
  Short bytes_read = fread(buffer, 1, bytes_to_read, file->fp);

  if (!bytes_read) {
    uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, 0);
    return;
  }
  
  if (bytes_read < bytes_to_read) {
    int diff = bytes_read - bytes_to_read;
    fseeko(file->fp, diff, SEEK_CUR);
  }

  Short write_addr = uxn_dev_read_short(uxn, page | FILE_READ_PORT);
  ENSURE_BUFFER_BOUNDS(write_addr, bytes_read);

  uxn_mem_load(uxn, buffer, bytes_read, write_addr);
  uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, bytes_read);
}

void file_write(Uxn *uxn, UxnFile *file, Byte page) {
  if (!file->fp) {
    uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, 0);
    return;
  }

  Short mem_addr = uxn_dev_read_short(uxn, page | FILE_WRITE_PORT);
  Short bytes_to_write = uxn_dev_read_short(uxn, page | FILE_LENGTH_PORT);
  Byte buffer[FILE_BUFFER_SIZE] = {0};

  ENSURE_BUFFER_BOUNDS(mem_addr, bytes_to_write);

  uxn_mem_buffer_read(uxn, bytes_to_write, buffer, mem_addr);
  Short bytes_written = fwrite(buffer, 1, bytes_to_write, file->fp);
  fflush(file->fp);

  uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, bytes_written);
}

void file_read_port_deo(Uxn *uxn, UxnFile *file, Byte page) {
  switch (file->state) {
  case STATE_INIT:
    if (file_open(file, STATE_READ)) {
      file->state = STATE_READ;
      file_read(uxn, file, page);
    }
    break;
  case STATE_READ:
    file_read(uxn, file, page);
    break;
  case STATE_WRITE:
    if (file_reopen(file, STATE_READ)) {
      file->state = STATE_READ;
      file_read(uxn, file, page);
    }
    break;
  case STATE_APPEND:
    if (file_reopen(file, STATE_READ)) {
      file->state = STATE_READ;
      file_read(uxn, file, page);
    }
    break;
  default:
    uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, 0);
    break;
  }
}

void file_write_port_deo(Uxn *uxn, UxnFile *file, Byte page) {
  Short append_mode = uxn_dev_read(uxn, page | FILE_APPEND_PORT);
  UxnFileState state = append_mode ? STATE_APPEND : STATE_WRITE;

  switch (file->state) {
  case STATE_INIT:
    if (file_open(file, state)) {
      file->state = state;
      file_write(uxn, file, page);
    }
    break;
  case STATE_READ:
    if (file_reopen(file, state)) {
      file->state = state;
      file_write(uxn, file, page);
    }
    break;
  case STATE_WRITE:
  case STATE_APPEND:
    if (file->state != state) {
      if (file_reopen(file, state)) {
        file->state = state;
        file_write(uxn, file, page);
      }
    } else {
      file_write(uxn, file, page);
    }
    break;
  default:
    uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, 0);
    break;
  }
}

void file_delete_port_deo(Uxn *uxn, UxnFile *file, Byte page) {
  remove(file->name);
  file_close(file);
}

void file_stat_error(char *buffer, Short buffer_len) {
  for (size_t i = 0; i < buffer_len; i++) {
    buffer[i] = '!';
  }
}

void file_stat(char *name, Short buffer_len, char *buffer,
               bool clip_large_size) {
  struct stat st;

  int err = stat(name, &st);

  if (err) {
    file_stat_error(buffer, buffer_len);
    return;
  }

  size_t size = st.st_size;

  if (clip_large_size && size > 0xffff) {
    for (size_t i = 0; i < buffer_len; i++)
      buffer[i] = '?';
    return;
  }

  const char digit_chars[16] = "0123456789abcdef";
  for (size_t i = 0; i < buffer_len; i++) {
    Byte digit = size & 0xf;
    buffer[buffer_len - (i + 1)] = digit_chars[digit];

    size >>= 4;
  }
}

void file_stat_port_deo(Uxn *uxn, UxnFile *file, Byte page) {
  Short buffer_addr = uxn_dev_read_short(uxn, page | FILE_STAT_PORT);
  Short buffer_len = uxn_dev_read_short(uxn, page | FILE_LENGTH_PORT);

  ENSURE_BUFFER_BOUNDS(buffer_addr, buffer_len);

  char *stat_buffer = calloc(buffer_len, sizeof(char));
  file_stat(file->name, buffer_len, stat_buffer, false);

  uxn_mem_load(uxn, (Byte *)stat_buffer, buffer_len, buffer_addr);

  uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, buffer_len);
  free(stat_buffer);
}

// Directory

void dir_init(UxnDir *dir, char *dirname) {
  *dir = (struct UxnDir){.dp = NULL,
                         .name = dirname,
                         .type = UXN_DIR_TYPE,
                         .content = NULL,
                         .read_offset = 0};
}

int dir_open(UxnDir *dir) {
  dir->dp = opendir(dir->name);
  dir->state = STATE_INIT;

  return dir->dp ? 1 : 0;
}

int dir_close(UxnDir *dir) {
  if (dir->dp) {
    closedir(dir->dp);
    dir->dp = NULL;
  }

  if (dir->name) {
    free(dir->name);
    dir->name = NULL;
  }

  if (dir->content) {
    free(dir->content);
    dir->content = NULL;
  }

  return 1;
}

int dir_reopen(UxnDir *dir) {
  if (dir->dp) {
    closedir(dir->dp);
    dir->dp = NULL;
  }

  if (dir->content) {
    free(dir->content);
    dir->content = NULL;
  }

  return dir_open(dir);
}

Short read_directory_entry(char *dirname, size_t max_size,
                           char buffer[max_size + 1]) {
  if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0) {
    return 0;
  }

  const int prefix_length = 4;
  const size_t name_len = strlen(dirname);
  if (max_size < prefix_length + name_len + 3)
    return 0;

  char *p = stpcpy(buffer, "---- ");
  p = stpcpy(p, dirname);
  p = stpcpy(p, "/\n");

  return p - buffer;
}

Short read_file_entry(char *path, char *filename, size_t max_size,
                      char buffer[max_size + 1]) {

  const int prefix_length = 4;
  const size_t name_len = strlen(filename);
  if (max_size < prefix_length + name_len + 2)
    return 0;

  const size_t path_len = strlen(path);
  char *full_path = malloc(path_len + name_len + 2);
  char *p = stpcpy(full_path, path);
  p = stpcpy(p, "/");
  p = stpcpy(p, filename);
  file_stat(full_path, prefix_length, buffer, true);
  free(full_path);

  p = stpcpy(buffer + 4, " ");
  p = stpcpy(p, filename);
  p = stpcpy(p, "\n");

  return p - buffer;
}

void dir_read_continue(Uxn *uxn, UxnDir *dir, Byte page) {
  Short write_addr = uxn_dev_read_short(uxn, page | FILE_READ_PORT);
  Short write_len = uxn_dev_read_short(uxn, page | FILE_LENGTH_PORT);

  ENSURE_BUFFER_BOUNDS(write_addr, write_len);

  size_t content_length = strlen(dir->content);
  if (dir->read_offset + write_len >= content_length) {
    write_len = content_length - dir->read_offset;
  }

  if (write_len == 0) {
    uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, 0);
    return;
  }

  uxn_mem_load(uxn, (Byte *)dir->content + dir->read_offset, write_len,
               write_addr);

  dir->read_offset += write_len;
  uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, write_len);
}

int filter_dot_directories(const struct dirent *ep) {
  return !(strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0);
}

void dir_read(Uxn *uxn, UxnDir *dir, Byte page) {

  if (!dir->dp) {
    uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, 0);
    return;
  }

  if (dir->content) {
    dir_read_continue(uxn, dir, page);
    return;
  }

  struct dirent **entries = NULL;
  int num_entries =
      scandir(dir->name, &entries, filter_dot_directories, alphasort);

  if (num_entries == -1) {
    perror("Error reading directory:");
    exit(EXIT_FAILURE);
  }

  int content_length = 0;
  char **lines = calloc(num_entries, sizeof(*lines));
  char buffer[MAX_FILE_NAME_LENGTH + 1] = {0};
  for (int i = 0; i < num_entries; i++) {
    memset(buffer, 0, MAX_FILE_NAME_LENGTH + 1);

    switch (entries[i]->d_type) {
    case DT_DIR:
      read_directory_entry(entries[i]->d_name, MAX_FILE_NAME_LENGTH, buffer);
      break;
    case DT_REG:
      read_file_entry(dir->name, entries[i]->d_name, MAX_FILE_NAME_LENGTH,
                      buffer);
      break;
    default:
      continue;
    }

    int line_length = strlen(buffer);
    content_length += line_length;
    lines[i] = calloc(line_length + 1, sizeof(char));
    strcpy(lines[i], buffer);
  }

  char *content = calloc(content_length + 1, sizeof(char));
  char *p = content;

  for (int i = 0; i < num_entries; i++) {
    p = stpcpy(p, lines[i]);
  }

  dir->content = content;
  dir->read_offset = 0;

  for (int i = 0; i < num_entries; i++) {
    free(lines[i]);
    free(entries[i]);
  }

  free(lines);

  dir_read_continue(uxn, dir, page);
}

void dir_read_port_deo(Uxn *uxn, UxnDir *dir, Byte page) {
  switch (dir->state) {
  case STATE_INIT:
    if (dir_open(dir)) {
      dir->state = STATE_READ;
      dir_read(uxn, dir, page);
    }
    break;
  case STATE_READ:
    dir_read(uxn, dir, page);
    break;
  default:
    uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, 0);
    break;
  }
}

void dir_stat_port_deo(Uxn *uxn, UxnDir *dir, Byte page) {
  Short buffer_addr = uxn_dev_read_short(uxn, page | FILE_STAT_PORT);
  Short buffer_len = uxn_dev_read_short(uxn, page | FILE_LENGTH_PORT);

  ENSURE_BUFFER_BOUNDS(buffer_addr, buffer_len);

  for (size_t i = 0; i < buffer_len; i++) {
    uxn_mem_write(uxn, buffer_addr + i, '-');
  }

  uxn_dev_write_short(uxn, page | FILE_SUCCESS_PORT, buffer_len);
}

// Stream

void ensure_parent_directory_exists(char *pathname) {
  const size_t len = strlen(pathname);
  char strbuf[len + 1];
  strbuf[len] = '\0';
  
  const char *last_slash = strrchr(pathname, '/');
  char *p = pathname;
  while (p && p < last_slash) {
    p = strchr(p + 1, '/');
    if (p) {
      strncpy(strbuf, pathname, p - pathname);
      strbuf[p - pathname] = '\0';
      if (stat(strbuf, &(struct stat){0}) == -1) {
        if (mkdir(strbuf, 0777) == -1) {
          perror("mkdir");
          exit(EXIT_FAILURE);
        }
      }

    }
  }
}

int stream_init(UxnStream *stream, char *name) {
  struct stat st;

  int err = stat(name, &st);

  if (err) {
    // File doesn't exist, so create it.
    ensure_parent_directory_exists(name);
    file_init(&stream->file, name);
    return 1;
  }

  if (S_ISDIR(st.st_mode)) {
    dir_init(&stream->dir, name);
  } else if (S_ISREG(st.st_mode)) {
    file_init(&stream->file, name);
  } else {
    return 0;
  }

  return 1;
}

void stream_close(UxnStream *stream) {
  switch (stream->file.type) {
  case UXN_FILE_TYPE:
    file_close(&stream->file);
    break;
  case UXN_DIR_TYPE:
    dir_close(&stream->dir);
    break;
  default:
    break;
  }
}

void file_name_port_deo(Uxn *uxn, UxnStream *stream, Byte page) {
  stream_close(stream);
  Short name_addr = uxn_dev_read_short(uxn, page | FILE_NAME_PORT);
  char *name = read_filename(uxn, name_addr);
  stream_init(stream, name);
}

void file_deo(Uxn *uxn, Byte addr) {
  UxnStream *streams = uxn_get_open_files(uxn);

  if (!streams) {
    streams = calloc(FILE_COUNT, sizeof(union UxnStream));

    if (streams) {
      streams[FILE_A] = (union UxnStream){0};
      streams[FILE_B] = (union UxnStream){0};
      uxn_set_open_files(uxn, streams);
    } else {
      uxn_set_open_files(uxn, NULL);
      printf("Failed to allocate memory for files\n");
    }
  }

  UxnStream *stream = &streams[FILE_IDX(addr)];
  const Byte page = addr & 0xf0;
  const Byte port = addr & 0x0f;

  switch (port) {
  case FILE_NAME_PORT:
    file_name_port_deo(uxn, stream, page);
    break;
  case FILE_READ_PORT:
    switch (stream->file.type) {
    case UXN_FILE_TYPE:
      file_read_port_deo(uxn, &stream->file, page);
      break;
    case UXN_DIR_TYPE:
      dir_read_port_deo(uxn, &stream->dir, page);
      break;
    }
    break;
  case FILE_WRITE_PORT:
    file_write_port_deo(uxn, &stream->file, page);
    break;
  case FILE_DELETE_PORT:
    file_delete_port_deo(uxn, &stream->file, page);
    break;
  case FILE_STAT_PORT:
    switch (stream->file.type) {
    case UXN_FILE_TYPE:
      file_stat_port_deo(uxn, &stream->file, page);
      break;
    case UXN_DIR_TYPE:
      dir_stat_port_deo(uxn, &stream->dir, page);
      break;
    }
    break;
  default:
    break;
  }
}

Byte file_dei(Uxn *uxn, Byte addr) { return uxn_dev_read(uxn, addr); }
