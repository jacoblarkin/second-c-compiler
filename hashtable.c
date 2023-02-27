#include "compiler.h"
#include "array.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_MAX_LOAD 0.75

#define table_capacity(t) ((struct HashTableHeader*)(t) - 1)->capacity
#define table_filled(t)   ((struct HashTableHeader*)(t) - 1)->filled

static PreprocessorTable preproc_table_create_with_capacity(uint64_t capacity) {
  PreprocessorTable t = (void*)(
    (struct HashTableHeader*)malloc(sizeof(struct HashTableHeader)
                                    + capacity*sizeof(struct KeyValueStrView))
     + 1);

  memset(t, 0, capacity*sizeof(struct KeyValueStrView));

  table_capacity(t) = capacity;
  table_filled(t)   = 0;

  return t;
}

static MacroTable macro_table_create_with_capacity(uint64_t capacity) {
  MacroTable t = (void*)(
   (struct HashTableHeader*)malloc(sizeof(struct HashTableHeader) 
                                   + capacity*sizeof(struct KeyValueMacro)) 
   + 1);

  memset(t, 0, capacity*sizeof(struct KeyValueMacro));

  table_capacity(t) = capacity;
  table_filled(t)   = 0;

  return t;
}

PreprocessorTable preproc_table_create() {
  return preproc_table_create_with_capacity(16);
}

MacroTable macro_table_create() {
  return macro_table_create_with_capacity(16);
}

void preproc_table_destroy(PreprocessorTable t) {
  free((struct HashTableHeader*)t - 1);
}

void macro_table_destroy(MacroTable t) {
  for(uint64_t i = 0; i < table_capacity(t); i++) {
    struct KeyValueMacro* entry = &t[i];
    if(entry->value.arg_names == NULL) continue;
    array_free(entry->value.arg_names);
  }

  free((struct HashTableHeader*)t - 1);
}

static struct KeyValueStrView*
find_entry(PreprocessorTable t, struct string_view key)
{
  uint64_t index = strview_hash(key) & (table_capacity(t)-1);
  struct KeyValueStrView* tombstone = NULL;
  while(true) {
    struct KeyValueStrView* entry = &t[index];
    if(entry->key.begin == NULL) {
      if(entry->key.length == 0) { // Empty entry
        return tombstone != NULL ? tombstone : entry;
      } else { // Found tombstone
        if(tombstone == NULL) tombstone = entry;
      }
    } else if(!strviewcmp(entry->key, key)) {
      return entry;
    }
    index = (index + 1) & (table_capacity(t) - 1);
  }
}

static struct KeyValueMacro*
macro_table_find_entry(MacroTable t, struct string_view key)
{
  uint64_t index = strview_hash(key) & (table_capacity(t)-1);
  struct KeyValueMacro* tombstone = NULL;
  while(true) {
    struct KeyValueMacro* entry = &t[index];
    if(entry->key.begin == NULL) {
      if(entry->key.length == 0) { // Empty entry
        return tombstone != NULL ? tombstone : entry;
      } else { // Found tombstone
        if(tombstone == NULL) tombstone = entry;
      }
    } else if(!strviewcmp(entry->key, key)) {
      return entry;
    }
    index = (index + 1) & (table_capacity(t) - 1);
  }
}

struct string_view preproc_table_get(PreprocessorTable t,
                                     struct string_view key) {
  if(table_filled(t) == 0) return (struct string_view){.begin=NULL, .length=0}; 

  struct KeyValueStrView *entry = find_entry(t, key);
  if(entry == NULL) return (struct string_view){.begin=NULL, .length=0};

  return entry->value;
}

struct Macro macro_table_get(MacroTable t, struct string_view key) {
  struct Macro null_macro = { .text = {0}, .arg_names = NULL };
  if(table_filled(t) == 0) return null_macro; 

  struct KeyValueMacro *entry = macro_table_find_entry(t, key);
  if(entry == NULL) return null_macro;

  return entry->value;
}

static void adjust_capacity(PreprocessorTable* t, uint64_t capacity) {
  PreprocessorTable newT = preproc_table_create_with_capacity(capacity);
  
  for(uint64_t i = 0; i < table_capacity(*t); i++) {
    struct KeyValueStrView* entry = &(*t)[i];
    if(entry->key.begin == NULL) continue;

    struct KeyValueStrView* dst = find_entry(newT, entry->key);
    dst->key = entry->key;
    dst->value = entry->value;
    table_filled(newT)++;
  }

  preproc_table_destroy(*t);
  *t = newT;
}

static void macro_table_adjust_capacity(MacroTable* t, uint64_t capacity) {
  MacroTable newT = macro_table_create_with_capacity(capacity);
  
  for(uint64_t i = 0; i < table_capacity(*t); i++) {
    struct KeyValueMacro* entry = &(*t)[i];
    if(entry->key.begin == NULL) continue;

    struct KeyValueMacro* dst = macro_table_find_entry(newT, entry->key);
    dst->key = entry->key;
    dst->value = macro_copy(entry->value);
    table_filled(newT)++;
  }

  macro_table_destroy(*t);
  *t = newT;
}

_Bool preproc_table_set(PreprocessorTable* t,
                        struct string_view key,
                        struct string_view value) {
  if(table_filled(*t) + 1 > (double)table_capacity(*t) * TABLE_MAX_LOAD) {
    uint64_t capacity = 2 * table_capacity(*t);
    adjust_capacity(t, capacity); 
  }

  struct KeyValueStrView *entry = find_entry(*t, key);
  bool isNewKey = entry->key.begin == NULL;
  if(isNewKey) table_filled(*t)++;

  entry->key = key;
  entry->value = value;
  return isNewKey;
}

bool macro_table_set(MacroTable* t, struct string_view key,
                     struct Macro value) {
  if(table_filled(*t) + 1 > (double)table_capacity(*t) * TABLE_MAX_LOAD) {
    uint64_t capacity = 2 * table_capacity(*t);
    macro_table_adjust_capacity(t, capacity); 
  }

  struct KeyValueMacro *entry = macro_table_find_entry(*t, key);
  bool isNewKey = entry->key.begin == NULL;
  if(isNewKey) table_filled(*t)++;

  if(!isNewKey && !entry->value.arg_names) array_free(entry->value.arg_names);

  entry->key = key;
  entry->value = macro_copy(value);
  return isNewKey;
}

_Bool preproc_table_delete(PreprocessorTable* t,
                           struct string_view key) {
  if(table_filled(*t) == 0) return false;

  struct KeyValueStrView* entry = find_entry(*t, key);
  if(entry->key.begin == NULL) return false;

  entry->key.begin = NULL;
  entry->key.length = 1; // Tombstone
  entry->value = (struct string_view){ .begin = NULL, .length = 0 };
  return true;
}

bool macro_table_delete(MacroTable* t, struct string_view key) {
  if(table_filled(*t) == 0) return false;

  struct KeyValueMacro* entry = macro_table_find_entry(*t, key);
  if(entry->key.begin == NULL) return false;

  entry->key.begin = NULL;
  entry->key.length = 1; // Tombstone
  array_free(entry->value.arg_names);
  entry->value = (struct Macro){0};
  return true;
}



