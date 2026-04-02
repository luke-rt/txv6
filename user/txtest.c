// user/txtest.c
#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "user/user.h"

#define TESTFILE "txdemo.txt"

static void write_all(int fd, const char *s) {
  int n = strlen(s);
  if (write(fd, s, n) != n) {
    printf("write failed\n");
    exit(1);
  }
}

static void reset_file(const char *path, const char *contents) {
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

static void print_file(const char *tag, const char *path) {
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

static void child_print(const char *tag, const char *path) {
  int pid = fork();
  int st = 0;

  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    printf("child txstatus -> %d\n", txstatus());
    print_file(tag, path);
    exit(0);
  }

  wait(&st);
}

int main(void) {
  int fd;
  int r;

  reset_file(TESTFILE, "ORIGINAL");
  print_file("initial stable", TESTFILE);

  r = txbegin();
  printf("txbegin -> %d\n", r);
  if (r < 0)
    exit(1);

  fd = open(TESTFILE, O_RDWR);
  if (fd < 0) {
    printf("open(O_RDWR) failed\n");
    txabort();
    exit(1);
  }

  // overwrite from offset 0
  write_all(fd, "SHADOW!!");
  close(fd);

  // Read in the same transaction context: should see shadow data.
  printf("txstatus -> %d\n", txstatus());
  print_file("inside tx (shadow)", TESTFILE);

  // Read from a separate process before commit: should see old stable data.
  child_print("outside tx before commit (stable)", TESTFILE);

  r = txcommit();
  printf("txcommit -> %d\n", r);
  if (r < 0)
    exit(1);

  // After commit, stable state should reflect the transaction writes.
  child_print("outside tx after commit (stable)", TESTFILE);
  printf("txstatus -> %d\n", txstatus());
  print_file("outside tx after commit (stable)", TESTFILE);

  unlink(TESTFILE);
  exit(0);
}
