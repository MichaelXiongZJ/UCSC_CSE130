#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BLOCK_SIZE 4096  // buffer size 4KiB

int code = 0;

int read_and_write(int fd, char* buffer) {
  ssize_t bytes_read, bytes_written;
  char* temp;
  while ((bytes_read = read(fd, buffer, BLOCK_SIZE)) >
         0) {  // while there's bytes left
    temp = buffer;
    while ((bytes_written = write(STDOUT_FILENO, temp, bytes_read)) !=
           bytes_read) {  // write to stdout
      if (bytes_written == -1) {
        return -1;
      } else {
        temp += bytes_written;
        bytes_read -= bytes_written;
      }
    }
  }
  return bytes_read;
}

int main(int argc, char* argv[]) {
  char* buffer = calloc(BLOCK_SIZE, sizeof(char));
  if (argc == 1) {                                     // if no arg
    if (read_and_write(STDIN_FILENO, buffer) == -1) {  // if read fails
      warn("stdin");
      return 1;
    }
  } else {
    for (int i = 1; i < argc; i++) {  // loop thru args
      char* input = argv[i];
      int fd;
      if (strcmp(input, "-")) {  // input is not "-"
        fd = open(input, O_RDONLY);
        if (fd == -1) {  // file DNE
          warn("%s", input);
          code = 1;
          continue;
        }
      } else {  // input is "-", stdin
        fd = STDIN_FILENO;
      }
      if (read_and_write(fd, buffer) == -1) {  // read fails
        warn("%s", input);
        code = 1;
        continue;
      }
      if (fd != 0) {
        close(fd);
      }
    }
  }
  free(buffer);
  return code;
}
