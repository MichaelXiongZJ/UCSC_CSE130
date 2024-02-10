#pragma once

#include <stddef.h>

#include "kvlist.h"

struct mapper_args {
  kvlist_t* input;
  kvlist_t* list;
  void (*mapper)(kvpair_t*, kvlist_t*);
};

struct reducer_args {
  kvlist_t* list;
  kvlist_t* output;
  void (*reducer)(char*, kvlist_t*, kvlist_t*);
};

// mapper_args *mapper_args_new(kvpair_t *pair, kvlist_t *output, mapper_t
// mapper);

typedef void (*mapper_t)(kvpair_t* kv, kvlist_t* output);
typedef void (*reducer_t)(char* key, kvlist_t* list, kvlist_t* output);

void map_reduce(mapper_t mapper, size_t num_mapper, reducer_t reducer,
                size_t num_reducer, kvlist_t* input, kvlist_t* output);

#define UNUSED(x) (void)(x)
