#include "mr.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"
#include "kvlist.h"

void print_list_of_list(size_t size, kvlist_t** list) {
  if (list) {
    for (size_t i = 0; i < size; i++) {
      printf("**list number %zu:**\n", i);
      if (list + i) {
        kvlist_print(1, list[i]);
      } else {
        printf("[empty]\n");
      }
    }
  } else {
    printf("The whole list of list DNE\n");
  }
}

void split(size_t num, kvlist_t* input, kvlist_t** output) {
  // loop thru input and add into each kvlist
  kvlist_iterator_t* itor = kvlist_iterator_new(input);
  size_t count = 0;
  while (1) {
    kvpair_t* pair = kvlist_iterator_next(itor);
    if (pair == NULL) {
      break;
    }
    // add the kvpair to a list
    if (count == num) {
      count -= num;
    }
    kvlist_append(output[count], kvpair_clone(pair));
    count++;
  }
  kvlist_iterator_free(&itor);
}

void* map_caller(void* arg) {
  struct mapper_args* args = (struct mapper_args*)arg;
  if (args->input) {
    kvlist_iterator_t* itor = kvlist_iterator_new(args->input);
    while (1) {
      kvpair_t* pair = kvlist_iterator_next(itor);
      if (pair == NULL) {
        break;
      }
      args->mapper(pair, args->list);
    }
    kvlist_iterator_free(&itor);
  }
  return NULL;
}

void* reduce_caller(void* arg) {
  struct reducer_args* args = (struct reducer_args*)arg;
  if (args->list) {
    kvlist_t* clean_list = kvlist_new();
    kvlist_iterator_t* itor = kvlist_iterator_new(args->list);
    char* key = NULL;
    while (1) {
      kvpair_t* pair = kvlist_iterator_next(itor);
      if (pair == NULL) {
        if (key) {
          args->reducer(key, clean_list, args->output);
        }
        kvlist_free(&clean_list);
        break;
      }
      if (!key) {  // first key
        key = pair->key;
      }
      if (strcmp(key, pair->key) != 0) {
        args->reducer(key, clean_list, args->output);
        key = pair->key;
        kvlist_renew(clean_list);
      }
      kvlist_append(clean_list, kvpair_clone(pair));
    }
    kvlist_iterator_free(&itor);
  }
  return NULL;
}

void shuffle(int num_reducer, kvpair_t* pair, kvlist_t** list) {
  int value = hash(pair->key) % num_reducer;
  kvlist_append(list[value], kvpair_clone(pair));
}

void map_reduce(mapper_t mapper, size_t num_mapper, reducer_t reducer,
                size_t num_reducer, kvlist_t* input, kvlist_t* output) {
  // split input into num_mapper lists for each thread
  kvlist_t** split_list = (kvlist_t**)calloc(num_mapper, sizeof(kvlist_t*));
  for (size_t i = 0; i < num_mapper; i++) {
    split_list[i] = kvlist_new();
  }
  split(num_mapper, input, split_list);

  // printf("BEFORE:\n");
  // print_list_of_list(num_mapper, split_list);
  // spawn mapper threads in a loop and execute function.
  pthread_t mapper_threads[num_mapper];
  kvlist_t** mapped_list = (kvlist_t**)calloc(num_mapper, sizeof(kvlist_t*));
  struct mapper_args* map_arg_pair[num_mapper];
  for (size_t i = 0; i < num_mapper; i++) {
    mapped_list[i] = kvlist_new();
    // iterate thru each pair in perticular list
    map_arg_pair[i] = (struct mapper_args*)malloc(sizeof(struct mapper_args));
    map_arg_pair[i]->input = split_list[i];
    map_arg_pair[i]->list = mapped_list[i];
    map_arg_pair[i]->mapper = mapper;
    if (pthread_create(mapper_threads + i, NULL, map_caller, map_arg_pair[i])) {
      perror("Filed to create thread");
    }
  }

  for (size_t i = 0; i < num_mapper; i++) {
    if (pthread_join(mapper_threads[i], NULL) != 0) {
      perror("Failed to join thread");
    }
    kvlist_free(split_list + i);
    free(map_arg_pair[i]);
  }
  free(split_list);

  // hash each list and add them accroadinly to the shuffled_list
  kvlist_t** shuffled_list = (kvlist_t**)calloc(num_reducer, sizeof(kvlist_t*));
  for (size_t i = 0; i < num_reducer; i++) {
    shuffled_list[i] = kvlist_new();
  }
  for (size_t i = 0; i < num_mapper; i++) {
    kvlist_iterator_t* itor = kvlist_iterator_new(mapped_list[i]);
    while (1) {
      kvpair_t* pair = kvlist_iterator_next(itor);
      if (pair == NULL) {
        break;
      }
      shuffle(num_reducer, pair, shuffled_list);
    }
    kvlist_iterator_free(&itor);
  }
  for (size_t i = 0; i < num_mapper; i++) {
    kvlist_free(mapped_list + i);
  }
  free(mapped_list);

  // sort each kvlist
  for (size_t i = 0; i < num_reducer; i++) {
    kvlist_sort(shuffled_list[i]);
  }
  // reduce phase
  pthread_t reducer_threads[num_reducer];
  struct reducer_args* red_args[num_reducer];
  kvlist_t** reduced_list = (kvlist_t**)calloc(num_reducer, sizeof(kvlist_t*));
  for (size_t i = 0; i < num_reducer; i++) {
    reduced_list[i] = kvlist_new();
    red_args[i] = (struct reducer_args*)malloc(sizeof(struct reducer_args));
    red_args[i]->list = shuffled_list[i];
    red_args[i]->output = reduced_list[i];
    red_args[i]->reducer = reducer;
    if (pthread_create(reducer_threads + i, NULL, reduce_caller, red_args[i])) {
      perror("Filed to create reduce thread");
    }
  }
  for (size_t i = 0; i < num_reducer; i++) {
    if (pthread_join(reducer_threads[i], NULL) != 0) {
      perror("Failed to join reduce thread");
    }
    kvlist_free(shuffled_list + i);
    free(red_args[i]);
  }
  free(shuffled_list);

  // extend output with every reduced_list
  for (size_t i = 0; i < num_reducer; i++) {
    kvlist_extend(output, reduced_list[i]);
    kvlist_free(reduced_list + i);
  }
  free(reduced_list);
}
