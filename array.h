#ifndef Array_H
#define Array_H

#include <stdlib.h>
#include <string.h>

#define typeof __typeof__

struct ArrayHeader { unsigned long length, capacity;};
#define Array(T) T*
#define array_length(a) (((struct ArrayHeader*)(a)-1)->length)
#define array_capacity(a) (((struct ArrayHeader*)(a)-1)->capacity)

#define array_new() \
  (void*)((struct ArrayHeader*)malloc(sizeof(struct ArrayHeader)) + 1)

#define array_free(a) free((struct ArrayHeader*)(a) - 1)

#define array_ensure(a, c) \
  *(a) = (void*)((struct ArrayHeader*) realloc( (struct ArrayHeader*)(*(a)) - 1, \
    sizeof(struct ArrayHeader) + (array_capacity(*(a)) = (c))*sizeof(**(a)) ) + 1)

#define array_append(a, v) \
  (array_length(*(a)) >= array_capacity(*(a)) ? \
    (typeof(**(a))*)(array_ensure((a), array_capacity(*(a)) * 2)) \
    : *(a))[array_length(*(a))++] = (v)

#define array_pop(a) --array_length(a)

#endif
