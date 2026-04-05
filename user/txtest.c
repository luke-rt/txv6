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

static void test_commit(void) {
  int fd;
  int r;

  printf("=== test_commit ===\n");
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

  write_all(fd, "SHADOW!!");
  close(fd);

  printf("txstatus -> %d\n", txstatus());
  print_file("inside tx (shadow)", TESTFILE);
  child_print("outside tx before commit (stable)", TESTFILE);

  r = txcommit();
  printf("txcommit -> %d\n", r);
  if (r < 0)
    exit(1);

  child_print("outside tx after commit (stable)", TESTFILE);
  printf("txstatus -> %d\n", txstatus());
  print_file("after commit (stable)", TESTFILE);

  unlink(TESTFILE);
}

static void test_abort(void) {
  int fd;
  int r;

  printf("=== test_abort ===\n");
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

  write_all(fd, "SHADOW!!");
  close(fd);

  // Shadow write visible inside transaction
  print_file("inside tx (shadow)", TESTFILE);
  child_print("outside tx before abort (stable)", TESTFILE);

  r = txabort();
  printf("txabort -> %d\n", r);
  if (r < 0)
    exit(1);

  // After abort, file must show original content
  printf("txstatus -> %d\n", txstatus());
  print_file("after abort (ORIGINAL)", TESTFILE);
  child_print("child after abort (ORIGINAL)", TESTFILE);

  unlink(TESTFILE);
}

int main(void) {
  test_commit();
  test_abort();
  exit(0);
}
