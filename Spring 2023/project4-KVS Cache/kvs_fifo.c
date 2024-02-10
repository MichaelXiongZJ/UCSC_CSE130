#include "kvs_fifo.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char key[KVS_KEY_MAX];
  char value[KVS_VALUE_MAX];
  int dirty;
} Pair;

struct kvs_fifo {
  // TODO: add necessary variables
  kvs_base_t* kvs_base;
  int capacity;

  Pair* frames;  // the cache
  int front;
  int rear;
  int count;
};

kvs_fifo_t* kvs_fifo_new(kvs_base_t* kvs, int capacity) {
  kvs_fifo_t* kvs_fifo = malloc(sizeof(kvs_fifo_t));
  kvs_fifo->kvs_base = kvs;
  kvs_fifo->capacity = capacity;

  // TODO: initialize other variables
  kvs_fifo->frames = calloc(sizeof(Pair), capacity);
  kvs_fifo->front = -1;
  kvs_fifo->rear = -1;
  kvs_fifo->count = 0;

  return kvs_fifo;
}

void kvs_fifo_free(kvs_fifo_t** ptr) {
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

void enqueue(kvs_fifo_t* page, const char* key, const char* value, bool dirty) {
  if (page->front == -1) {
    page->front = 0;
  }
  page->rear = (page->rear + 1) % page->capacity;
  strcpy(page->frames[page->rear].key, key);
  strcpy(page->frames[page->rear].value, value);
  page->frames[page->rear].dirty = dirty;
  page->count++;
}

void dequeue(kvs_fifo_t* page) {
  if (page->frames[page->front].dirty) {  // write back if dirty
    kvs_base_set(page->kvs_base, page->frames[page->front].key,
                 page->frames[page->front].value);
  }
  page->front = (page->front + 1) % page->capacity;
  page->count--;
}

bool isFull(kvs_fifo_t* page) {
  if (page->count == page->capacity) {
    return true;
  }
  return false;
}

// perform FIFO, evict oldest and bring in new pair
void update(kvs_fifo_t* page, const char* key, const char* value, bool dirty) {
  if (isFull(page)) {
    dequeue(page);
  }
  enqueue(page, key, value, dirty);
}

int kvs_fifo_set(kvs_fifo_t* kvs_fifo, const char* key, const char* value) {
  // TODO: implement this function
  // search through the array for the key
  for (int i = 0; i < kvs_fifo->capacity; i++) {
    if (kvs_fifo->frames + i == NULL) {  // reached end
      break;
    }
    if (strcmp(kvs_fifo->frames[i].key, key) == 0) {  // key found in cache
      strcpy(kvs_fifo->frames[i].value, value);       // write in new value
      return SUCCESS;
    }
  }
  // if key not present, update the cache
  update(kvs_fifo, key, value, true);
  return SUCCESS;
}

int kvs_fifo_get(kvs_fifo_t* kvs_fifo, const char* key, char* value) {
  // TODO: implement this function
  // find the key in frame
  for (int i = 0; i < kvs_fifo->capacity; i++) {
    if (kvs_fifo->frames + i == NULL) {  // reached end
      break;
    }
    if (strcmp(kvs_fifo->frames[i].key, key) == 0) {  // key found in cache
      strcpy(value, kvs_fifo->frames[i].value);       // return value
      // value = kvs_fifo->frames[i].value;
      return SUCCESS;
    }
  }
  // if not in frames
  kvs_base_get(kvs_fifo->kvs_base, key, value);
  if (strcmp(value, "") == 0) {  // no such file found
    return FAILURE;
  }
  update(kvs_fifo, key, value, false);  // cache it
  return SUCCESS;
}

int kvs_fifo_flush(kvs_fifo_t* kvs_fifo) {
  // TODO: implement this function
  for (int i = 0; i < kvs_fifo->capacity; i++) {
    if (kvs_fifo->frames + i == NULL) {  // reached end
      break;
    }
    if (kvs_fifo->frames[i].dirty) {  // write the dirty ones
      kvs_base_set(kvs_fifo->kvs_base, kvs_fifo->frames[i].key,
                   kvs_fifo->frames[i].value);
      kvs_fifo->frames[i].dirty = 0;
    }
  }
  return SUCCESS;
}
