#define _GNU_SOURCE

#include <err.h>
#include <errno.h>
#include <linux/limits.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <wait.h>

#include "change_root.h"

#define CONTAINER_ID_MAX 16
#define CHILD_STACK_SIZE 4096 * 10

typedef struct container {
  char id[CONTAINER_ID_MAX];
  // TODO: Add fields
  char* image;
  char** cmd;
  char cwd[PATH_MAX];
} container_t;

/**
 * `usage` prints the usage of `client` and exists the program with
 * `EXIT_FAILURE`
 */
void usage(char* cmd) {
  printf("Usage: %s [ID] [IMAGE] [CMD]...\n", cmd);
  exit(EXIT_FAILURE);
}

/**
 * `container_exec` is an entry point of a child process and responsible for
 * creating an overlay filesystem, calling `change_root` and executing the
 * command given as arguments.
 */
int container_exec(void* arg) {
  container_t* container = (container_t*)arg;
  // this line is required on some systems
  if (mount("/", "/", "none", MS_PRIVATE | MS_REC, NULL) < 0) {
    err(1, "mount / private");
  }

  // TODO: Create a overlay filesystem
  // `lowerdir`  should be the image directory: ${cwd}/images/${image}
  // `upperdir`  should be `/tmp/container/{id}/upper`
  // `workdir`   should be `/tmp/container/{id}/work`
  // `merged`    should be `/tmp/container/{id}/merged`
  // ensure all directories exist (create if not exists) and
  // call `mount("overlay", merged, "overlay", MS_RELATIME,
  //    lowerdir={lowerdir},upperdir={upperdir},workdir={workdir})`
  char container_dir[PATH_MAX], lowerdir[PATH_MAX], upperdir[PATH_MAX],
      workdir[PATH_MAX], merged[PATH_MAX];
  struct stat st = {0};
  sprintf(container_dir, "/tmp/container/%s", container->id);
  if (stat(container_dir, &st) == -1) {
    if (mkdir(container_dir, 0700) < 0) {
      err(errno, "Failed to create container directory");
    }
  }

  sprintf(lowerdir, "%s/images/%s", container->cwd, container->image);
  sprintf(upperdir, "/tmp/container/%s/upper", container->id);
  sprintf(workdir, "/tmp/container/%s/work", container->id);
  sprintf(merged, "/tmp/container/%s/merged", container->id);

  if (stat(upperdir, &st) == -1) {
    if (mkdir(upperdir, 0700) < 0) {
      err(errno, "Failed to create upper directory");
    }
  }
  if (stat(workdir, &st) == -1) {
    if (mkdir(workdir, 0700) < 0) {
      err(errno, "Failed to create work directory");
    }
  }
  if (stat(merged, &st) == -1) {
    if (mkdir(merged, 0700) < 0) {
      err(errno, "Failed to create merged directory");
    }
  }

  printf("lowerdir: %s\n", lowerdir);
  printf("upperdir: %s\n", upperdir);
  printf("workdir: %s\n", workdir);
  printf("merged: %s\n", merged);

  char data[PATH_MAX * 3];
  sprintf(data, "lowerdir=%s,upperdir=%s,workdir=%s", lowerdir, upperdir,
          workdir);
  if (mount("overlay", merged, "overlay", MS_RELATIME, data) != 0) {
    err(errno, "Failed to mount overlay");
  }

  // TODO: Call `change_root` with the `merged` directory
  change_root(merged);

  // TODO: use `execvp` to run the given command and return its return value
  return execvp(container->cmd[0], container->cmd);
}

int main(int argc, char** argv) {
  if (argc < 4) {
    usage(argv[0]);
  }

  /* Create tmpfs and mount it to `/tmp/container` so overlayfs can be used
   * inside Docker containers */
  if (mkdir("/tmp/container", 0700) < 0 && errno != EEXIST) {
    err(1, "Failed to create a directory to store container file systems");
  }
  if (errno != EEXIST) {
    if (mount("tmpfs", "/tmp/container", "tmpfs", 0, "") < 0) {
      err(1, "Failed to mount tmpfs on /tmp/container");
    }
  }

  /* cwd contains the absolute path to the current working directory which can
   * be useful constructing path for image */
  char cwd[PATH_MAX];
  getcwd(cwd, PATH_MAX);

  container_t container;
  strncpy(container.id, argv[1], CONTAINER_ID_MAX);

  // TODO: store all necessary information to `container`
  container.image = argv[2];
  container.cmd = &argv[3];
  strncpy(container.cwd, cwd, PATH_MAX);

  /* Use `clone` to create a child process */
  char child_stack[CHILD_STACK_SIZE];  // statically allocate stack for child
  int clone_flags = SIGCHLD | CLONE_NEWNS | CLONE_NEWPID;
  int pid = clone(container_exec, &child_stack, clone_flags, &container);
  if (pid < 0) {
    err(1, "Failed to clone");
  }

  waitpid(pid, NULL, 0);
  return EXIT_SUCCESS;
}
