#include "kvs_clock.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char key[KVS_KEY_MAX];
  char value[KVS_VALUE_MAX];
  int dirty;
  int reference;
} Pair;

struct kvs_clock {
  // TODO: add necessary variables
  kvs_base_t* kvs_base;
  int capacity;

  Pair* frames;  // the cache
  int count;
  int hand;
};

kvs_clock_t* kvs_clock_new(kvs_base_t* kvs, int capacity) {
  kvs_clock_t* kvs_clock = malloc(sizeof(kvs_clock_t));
  kvs_clock->kvs_base = kvs;
  kvs_clock->capacity = capacity;

  // TODO: initialize other variables
  kvs_clock->frames = calloc(sizeof(Pair), capacity);
  kvs_clock->hand = 0;
  kvs_clock->count = 0;

  return kvs_clock;
}

void kvs_clock_free(kvs_clock_t** ptr) {
  // TODO: free dynamically allocated memory
  if (*ptr != NULL) {
    if ((*ptr)->frames != NULL) {
      free((*ptr)->frames);
      (*ptr)->frames = NULL;
    }
    free(*ptr);
    *ptr = NULL;
  }
}

void clock_update(kvs_clock_t* page, const char* key, const char* value,
                  bool dirty) {
  // if full, free one
  int index = page->count;
  if (page->count >= page->capacity) {
    while (page->frames[page->hand].reference == 1) {  // loop til a freeable
                                                       // one
      page->frames[page->hand].reference = 0;
      page->hand++;
      if (page->hand == page->capacity) {
        page->hand = 0;
      }
    }
    // hand points at freeable
    if (page->frames[page->hand].dirty == 1) {  // write back
      kvs_base_set(page->kvs_base, page->frames[page->hand].key,
                   page->frames[page->hand].value);
    }
    index = page->hand;
  } else {
    page->count++;
  }
  // now load new into cache at index
  strcpy(page->frames[index].key, key);
  strcpy(page->frames[index].value, value);
  page->frames[index].dirty = dirty;
  page->frames[index].reference = 1;
}

int kvs_clock_set(kvs_clock_t* kvs_clock, const char* key, const char* value) {
  // TODO: implement this function
  for (int i = 0; i < kvs_clock->count; i++) {
    if (strcmp(kvs_clock->frames[i].key, key) == 0) {
      if (kvs_clock->frames[i].dirty == 1) {  // write back if it's dirty
        kvs_base_set(kvs_clock->kvs_base, kvs_clock->frames[i].key,
                     kvs_clock->frames[i].value);
      }
      strcpy(kvs_clock->frames[i].value, value);
      kvs_clock->frames[i].dirty = true;
      kvs_clock->frames[i].reference = 1;
      return SUCCESS;
    }
  }
  // if not found
  clock_update(kvs_clock, key, value, true);
  return SUCCESS;
}

int kvs_clock_get(kvs_clock_t* kvs_clock, const char* key, char* value) {
  // TODO: implement this function
  // find key, if present, get its value and update bit
  for (int i = 0; i < kvs_clock->count; i++) {
    if (strcmp(kvs_clock->frames[i].key, key) == 0) {
      strcpy(value, kvs_clock->frames[i].value);
      kvs_clock->frames[i].reference = 1;
      return SUCCESS;
    }
  }
  // if not found, call base. check failure as well
  kvs_base_get(kvs_clock->kvs_base, key, value);
  if (strcmp(value, "") == 0) {  // no such file found
    return FAILURE;
  }
  clock_update(kvs_clock, key, value, false);  // cache it
  return SUCCESS;
}

int kvs_clock_flush(kvs_clock_t* kvs_clock) {
  // TODO: implement this function
  // same as fifo
  for (int i = 0; i < kvs_clock->capacity; i++) {
    if (kvs_clock->frames + i == NULL) {  // reached end
      break;
    }
    if (kvs_clock->frames[i].dirty) {  // write the dirty ones
      kvs_base_set(kvs_clock->kvs_base, kvs_clock->frames[i].key,
                   kvs_clock->frames[i].value);
      kvs_clock->frames[i].dirty = 0;
    }
  }
  return SUCCESS;
}
