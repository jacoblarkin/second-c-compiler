#include "compiler.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned long strview_hash(struct string_view sv)
{
  const uint64_t fnv_offset_basis_64 = 0xcbf29ce484222325;
  const uint64_t fnv_prime_64 = 0x00000100000001B3;

  uint64_t hash = fnv_offset_basis_64;

  for(size_t i = 0; i < sv.length; i++) {
    hash ^= (uint8_t)sv.begin[i];
    hash *= fnv_prime_64;
  }

  return hash;
}

size_t array_sv_capacity(Array(struct string_view) array)
{
    return ((struct ArrayHeader*)array-1)->capacity;
}

void array_sv_ensure(Array(struct string_view)* array, size_t capacity)
{
    struct ArrayHeader* tmp = (struct ArrayHeader*)(*array) - 1;
    size_t new_size = sizeof(struct ArrayHeader)
        + sizeof(struct string_view) * capacity;

    tmp = realloc(tmp, new_size);

    *array = (Array(struct string_view))(tmp + 1);
    array_capacity(*array) = capacity;
}

void array_sv_append(Array(struct string_view)* array, struct string_view sv)
{
    if(array_length(*array) >= array_sv_capacity(*array)) {
        array_sv_ensure(array, 2 * array_sv_capacity(*array));
    }

    (*array)[array_length(*array)++] = sv;
}

int strviewcmp(struct string_view svL, struct string_view svR) {
  for(size_t i = 0; i < svL.length && i < svR.length; i++)
    if(svL.begin[i] != svR.begin[i])
      return svL.begin[i] - svR.begin[i];

  return (int)svL.length - (int)svR.length;
}

int strviewstrcmp(struct string_view strview, const char* str) {
  size_t i;

  for(i = 0; i < strview.length && str[i]; i++)
    if(strview.begin[i] != str[i]) 
      return strview.begin[i] - str[i];

  return (i != strview.length) + (-1) * (int)str[i]; 
}

char* strviewtostr(struct string_view sv) {
  char* ret = malloc((size_t)sv.length + 1);
  memcpy(ret, sv.begin, sv.length);
  ret[sv.length] = '\0';
  return ret;
}

void print_strview(struct string_view sv)
{
  for(size_t j = 0; j < sv.length; j++) 
    putc(sv.begin[j], stdout);

#ifdef DEBUG
    printf(" with size: %zu", sv.length);
#endif
}
