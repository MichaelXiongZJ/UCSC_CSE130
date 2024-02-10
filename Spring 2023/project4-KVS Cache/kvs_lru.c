#include "kvs_lru.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

unsigned int hash(const char* str) {
  unsigned int hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }
  return hash % KVS_KEY_MAX;
}

typedef struct Pair {
  char key[KVS_KEY_MAX];
  char value[KVS_VALUE_MAX];
  int dirty;
  struct Pair* prev;
  struct Pair* next;
} Pair;

struct kvs_lru {
  // TODO: add necessary variables
  kvs_base_t* kvs_base;
  int capacity;

  int size;
  Pair* hashmap[KVS_KEY_MAX];
  Pair* head;
  Pair* tail;
};

kvs_lru_t* kvs_lru_new(kvs_base_t* kvs, int capacity) {
  kvs_lru_t* kvs_lru = malloc(sizeof(kvs_lru_t));
  kvs_lru->kvs_base = kvs;
  kvs_lru->capacity = capacity;

  // TODO: initialize other variables
  kvs_lru->size = 0;
  // for(int i = 0; i < KVS_KEY_MAX; i++){
  //   kvs_lru->hashmap[i] = NULL;
  // }
  memset(kvs_lru->hashmap, 0, sizeof(Pair*) * KVS_KEY_MAX);
  Pair* head = (Pair*)malloc(sizeof(Pair));
  Pair* tail = (Pair*)malloc(sizeof(Pair));
  head->next = tail;
  head->prev = head;
  kvs_lru->head = head;
  kvs_lru->tail = tail;

  return kvs_lru;
}

void removeNode(kvs_lru_t* kvs, Pair* node) {
  Pair* prev = node->prev;
  Pair* next = node->next;
  prev->next = next;
  next->prev = prev;
}

void addToHead(kvs_lru_t* kvs_lru, Pair* node) {
  Pair* temp = kvs_lru->head->next;
  kvs_lru->head->next = node;
  node->prev = kvs_lru->head;
  node->next = temp;
  temp->prev = node;
}

void kvs_lru_free(kvs_lru_t** ptr) {
  // TODO: free dynamically allocated memory
  kvs_lru_t* kvs = *ptr;
  for (int i = 0; i < KVS_KEY_MAX; i++) {
    if (kvs->hashmap[i] != NULL) {
      free(kvs->hashmap[i]);
      kvs->hashmap[i] = NULL;
    }
  }
  free(kvs->head);
  free(kvs->tail);
  free(*ptr);
  *ptr = NULL;
}

void evict(kvs_lru_t* kvs) {
  Pair* to_evict = kvs->tail->prev;
  if (to_evict->dirty) {
    kvs_base_set(kvs->kvs_base, to_evict->key, to_evict->value);
  }
  kvs->hashmap[hash(to_evict->key)] = NULL;
  removeNode(kvs, to_evict);
  free(to_evict);
}

int kvs_lru_set(kvs_lru_t* kvs_lru, const char* key, const char* value) {
  // TODO: implement this function
  Pair* pair = kvs_lru->hashmap[hash(key)];
  if (pair) {  // if key already cached
    strcpy(pair->value, value);
    pair->dirty = 1;
    removeNode(kvs_lru, pair);
    addToHead(kvs_lru, pair);
    return SUCCESS;
  }
  // if not found
  if (kvs_lru->size >= kvs_lru->capacity) {  // full
    evict(kvs_lru);
  } else {
    kvs_lru->size++;
  }
  Pair* newPair = (Pair*)malloc(sizeof(Pair));
  strcpy(newPair->key, key);
  strcpy(newPair->value, value);
  newPair->dirty = 1;
  kvs_lru->hashmap[hash(newPair->key)] = newPair;
  addToHead(kvs_lru, newPair);
  return SUCCESS;
}

int kvs_lru_get(kvs_lru_t* kvs_lru, const char* key, char* value) {
  // TODO: implement this function
  Pair* pair = kvs_lru->hashmap[hash(key)];
  // if key cached
  if (pair) {
    strcpy(value, pair->value);
    removeNode(kvs_lru, pair);
    addToHead(kvs_lru, pair);
    return SUCCESS;
  }
  // if not cached
  kvs_base_get(kvs_lru->kvs_base, key, value);
  if (strcmp(value, "") == 0) {  // no such file found
    return FAILURE;
  }
  // add to cache
  if (kvs_lru->size >= kvs_lru->capacity) {  // full
    evict(kvs_lru);
  } else {
    kvs_lru->size++;
  }
  Pair* newPair = (Pair*)malloc(sizeof(Pair));
  strcpy(newPair->key, key);
  strcpy(newPair->value, value);
  newPair->dirty = 0;
  kvs_lru->hashmap[hash(newPair->key)] = newPair;
  addToHead(kvs_lru, newPair);
  return SUCCESS;
}

int kvs_lru_flush(kvs_lru_t* kvs_lru) {
  // TODO: implement this function
  Pair* current = kvs_lru->head->next;
  while (current != kvs_lru->tail) {
    if (current->dirty) {
      kvs_base_set(kvs_lru->kvs_base, current->key, current->value);
      current->dirty = 0;
    }
    current = current->next;
  }
  return FAILURE;
}
