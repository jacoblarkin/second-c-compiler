#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>

static _Noreturn void error(char* msg)
{
  puts(msg);
  abort();
}

int main(int argc, char* argv[])
{
  if(argc != 2) {
    error("Expected exactly 1 argument, the file to compile.");
  }
  printf("Hello, World! Will compile %s.\n", argv[1]);
  lexer_init(argv[1]);
  struct Token tok;
  get_next_token(&tok);
  return 0;
}
