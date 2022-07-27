#include "compiler.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* current_file;
char* buffer;
int line = 0;
int position = 0;

_Noreturn
static void error(const char* msg, ...)
{
  va_list ap;
  va_start(ap, msg);
  vfprintf(stderr, msg, ap);
  va_end(ap);
  abort();
}

void lexer_init(const char* filename)
{
  current_file = strdup(filename);
  if(!current_file) {
    error("Failed to allocate enough memory to store filename %s.\n", filename);
  }

  FILE* file = fopen(filename, "r");
  if(!file) {
    error("Did not find file %s.\n", filename);
  }

  int err = fseek(file, 0L, SEEK_END);
  if(err) {
    fclose(file);
    error("Error seeking end of file %s.\n", filename);
  }
  long ftell_ret = ftell(file);
  if(ftell_ret == -1L) {
    fclose(file);
    error("Error getting end position of file %s from ftell.\n", filename);
  }
  size_t size = (size_t)ftell_ret;
  rewind(file);

  buffer = malloc((size+1)*sizeof(char));
  if(!buffer) {
    fclose(file);
    error("Failed to allocate enough memory to read in file. Need %zu bytes.\n", size);
  }

  size_t bytes_read = fread(buffer, sizeof(char), size, file);
  int eof = feof(file);
  int ferr = ferror(file);
  fclose(file);
  if(bytes_read != size) {
    if(eof) {
      error("Unexpected EOF %f%% of the way through (%zu of %zu bytes).\n", 100.0 * ((double)bytes_read / (double)size), bytes_read, size);
    } else if (ferr){
      error("Error reading in %s.\n", filename);
    } else {
      error("Read in fewer bytes (%zu) than expected (%zu). No error or EOF reported.\n", bytes_read, size);
    }
  }
  buffer[size] = '\0';
}

_Bool get_next_token(struct Token* out)
{
  if(!out) return 0;

  out->file = strdup(current_file);
  out->line = line;
  out->position = position;
  out->type = UNKNOWN_TOK;
  out->value = (struct string_view) {.begin = NULL, .length = 0};

  return 1;
}
