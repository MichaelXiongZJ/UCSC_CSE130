#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "dining.h"
#include "utils.h"
// Tests for cleaners entering and not having to wait indefinitely if students
// arrive after (extra credit)
int main(void) {
  dining_t* d = dining_init(4);

  student_t student1 = make_student(1, d);
  cleaning_t cleaning1 = make_cleaning(1, d);
  cleaning_t cleaning2 = make_cleaning(2, d);
  cleaning_t cleaning3 = make_cleaning(3, d);

  //Student 1 enters, this blocks all cleaners.
  student_enter(&student1);

  msleep(100);

  //Cleaners 1 2 & 3 try to join. Are blocked by Student 1
  pthread_create(&cleaning1.thread, NULL, cleaning_enter, &cleaning1);
  msleep(100);
  pthread_create(&cleaning2.thread, NULL, cleaning_enter, &cleaning2);
  msleep(100);
  pthread_create(&cleaning3.thread, NULL, cleaning_enter, &cleaning3);
  msleep(100);


  //Student 1 leaves. Unblocks Cleaners.
  student_leave(&student1);

  msleep(100);

  // Cleaners leave in order 1 2 then 3
  pthread_join(cleaning1.thread, NULL);
  cleaning_leave(&cleaning1);
  msleep(100);

  pthread_join(cleaning2.thread, NULL);
  cleaning_leave(&cleaning2);
  msleep(100);

  pthread_join(cleaning3.thread, NULL);
  cleaning_leave(&cleaning3);


  dining_destroy(&d);
}
