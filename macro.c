#include "array.h"
#include "compiler.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

struct Macro macro_copy(struct Macro to_copy) {
  struct Macro copy = to_copy;
  copy.arg_names = array_new();
  array_capacity(copy.arg_names) = 0;
  array_length(copy.arg_names) = 0;
  array_sv_ensure(&copy.arg_names, array_length(to_copy.arg_names));
  for(unsigned long i = 0; i < array_length(to_copy.arg_names); i++) {
    array_sv_append(&(copy.arg_names), to_copy.arg_names[i]);
    //copy.arg_names[i] = to_copy.arg_names[i];
    //array_length(copy.arg_names)++;
  }
  return copy;
}

static inline int arg_index(Array(struct string_view) names, struct string_view name)
{
  for(size_t i = 0; i < array_length(names); ++i) {
      printf("Checking arg %zu: ", i);
      print_strview(names[i]);
      printf("\n");
    if(!strviewcmp(names[i], name)) {
        printf("Matched to arg number %zu\n", i);
        return (int)i;
    }
  }
  return -1;
}

char* macro_expand(struct Macro macro, Array(struct string_view) arguments)
{
  size_t length = 0;
  length += macro.text.length; // Will overestimate final length, but that's fine
  for(size_t i = 0; i < array_length(arguments); ++i) {
    length += arguments[i].length;
  }

  char* expansion = malloc(length * sizeof(char));

  size_t count = 0;
  bool inIdent = false;
  struct string_view ident = {0};
  for(size_t i = 0; i < macro.text.length; ++i) {
    char c = macro.text.begin[i];
    if((!isalnum(c) && c != '_') || (!inIdent && isdigit(c))) {
      if(inIdent) {
        int index = arg_index(macro.arg_names, ident);
        if(index >= 0) {
          if(count + arguments[index].length >= length) {
            length = 3 * length / 2;
            expansion = realloc(expansion, length);
          }
          strncpy(expansion + count, arguments[index].begin, arguments[index].length);
          count += arguments[index].length;
        } else {
          if(count + ident.length >= length) {
            length = 3 * length / 2;
            expansion = realloc(expansion, length);
          }
          strncpy(expansion + count, ident.begin, ident.length);
          count += ident.length;
        }
        inIdent = false;
        ident.length = 0;
      }
      if(count + 1 >= length) {
        length = 3 * length / 2;
        expansion = realloc(expansion, length);
      }
      expansion[count++] = c;
      continue;
    }
    inIdent = true;
    if(ident.length == 0) ident.begin = &macro.text.begin[i];
    ident.length++;
  }
  if(inIdent) {
    int index = arg_index(macro.arg_names, ident);
    if(index >= 0) {
      if(count + arguments[index].length >= length) {
        length = 2 * length;
        expansion = realloc(expansion, length);
      }
      strncpy(expansion + count, arguments[index].begin, arguments[index].length);
      count += arguments[index].length;
    } else {
      if(count + ident.length >= length) {
        length = 2 * length;
        expansion = realloc(expansion, length);
      }
      strncpy(expansion + count, ident.begin, ident.length);
      count += ident.length;
    }
  }
  
  if(count + 1 >= length) {
    length = count + 1;
    expansion = realloc(expansion, length);
  }
  expansion[count] = '\0';
  return expansion;
}

