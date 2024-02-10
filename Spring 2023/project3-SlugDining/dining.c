#include "dining.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

typedef struct dining {
  int capacity;
  sem_t space_left;             // current number of space left
  sem_t cleaning_sem;           // lock for cleaning
  pthread_cond_t student_cond;  // for signaling when cleaning is done
  pthread_cond_t cond_clean;    // for sigaling when a student can enter
} dining_t;

dining_t *dining_init(int capacity) {
  dining_t *dining = malloc(sizeof(dining_t));
  dining->capacity = capacity;
  sem_init(&dining->space_left, 0, capacity);
  sem_init(&dining->cleaning_sem, 0, 1);
  pthread_cond_init(&dining->cond_clean, NULL);
  pthread_cond_init(&dining->student_cond, NULL);
  return dining;
}

void dining_destroy(dining_t **dining) {
  sem_destroy(&(*dining)->space_left);
  pthread_cond_destroy(&(*dining)->cond_clean);
  pthread_cond_destroy(&(*dining)->student_cond);
  sem_destroy(&(*dining)->cleaning_sem);
  free(*dining);
  *dining = NULL;
}

void dining_student_enter(dining_t *dining) {
  sem_wait(
      &dining->cleaning_sem);  // checks if cleaning in progress, if so, wait
  sem_post(&dining->cleaning_sem);  // undo the change???
  sem_wait(&dining->space_left);    // check if there's space left
}

void dining_student_leave(dining_t *dining) {
  sem_post(&dining->space_left);
  pthread_cond_signal(&dining->cond_clean);
}

void dining_cleaning_enter(dining_t *dining) {
  sem_wait(&dining->cleaning_sem);              // wait for other cleaning
  for (int i = 0; i < dining->capacity; i++) {  // check if empty
    sem_wait(&dining->space_left);
  }
}

void dining_cleaning_leave(dining_t *dining) {
  for (int i = 0; i < dining->capacity; i++) {
    sem_post(&dining->space_left);
  }
  sem_post(&dining->cleaning_sem);                // cleaning over
  pthread_cond_broadcast(&dining->student_cond);  // signaling students
}
