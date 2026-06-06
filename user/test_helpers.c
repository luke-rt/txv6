#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "user/user.h"

#include "user/test_helpers.h"

// ================================================
// ***************** HELPERS **********************
// ================================================

// Function for writing to a file
void write_all(int fd, const char *s) {
  int n = strlen(s);
  if (write(fd, s, n) != n) {
    printf("write failed\n");
    exit(1);
  }
}

// Function for replacing current file contents with some default value
void reset_file(const char *path, const char *contents) {
  int fd;

  unlink(path);
  fd = open(path, O_CREATE | O_RDWR);
  if (fd < 0) {
    printf("open(create) failed\n");
    exit(1);
  }

  write_all(fd, contents);
  close(fd);
}

// Function for printing the contents of the file to the terminal
void print_file(const char *tag, const char *path) {
  char buf[128];
  int fd = open(path, O_RDONLY);
  int n;

  if (fd < 0) {
    printf("%s: open failed\n", tag);
    return;
  }

  n = read(fd, buf, sizeof(buf) - 1);
  if (n < 0) {
    printf("%s: read failed\n", tag);
    close(fd);
    return;
  }
  buf[n] = 0;

  printf("%s: \"%s\"\n", tag, buf);
  close(fd);
}

// Function for forking a child process to print tx status and file contents 
// (simulating an external observer), then waiting for it to finish.
void child_print(const char *tag, const char *path) {
  int pid = fork();
  int st = 0;

  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    printf("Child txstatus: %d\n", txstatus());
    print_file(tag, path);
    exit(0);
  }

  wait(&st);
}

// Function for sorting an array of integer values in-place.
// Primarily used for sorting latency/runtime data.
void sort(int *arr, int n) {
  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      if (arr[j] < arr[i]) {
        int tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
      }
    }
  }
}
