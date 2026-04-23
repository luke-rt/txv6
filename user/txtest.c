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

static void test_conflict(void) {
  int pid;
  int st = 0;
  int p2c[2], c2p[2];
  char buf[1];

  printf("=== test_conflict ===\n");
  reset_file(TESTFILE, "ORIGINAL");
  print_file("initial stable", TESTFILE);

  pipe(p2c);
  pipe(c2p);

  pid = fork();
  if (pid < 0) {
    printf("fork failed\n");
    exit(1);
  }

  if (pid == 0) {
    // ── child ──────────────────────────────────────────────────────
    close(p2c[1]);
    close(c2p[0]);

    if (txbegin() < 0) {
      printf("child: txbegin failed\n");
      exit(1);
    }
    printf("child: txbegin ok\n");

    int fd = open(TESTFILE, O_RDWR);
    if (fd < 0) {
      printf("child: open failed\n");
      txabort();
      exit(1);
    }
    write_all(fd, "CHILD!!!");
    close(fd);

    // tell parent we are inside tx and have written
    write(c2p[1], "r", 1);
    // wait for parent to also have written
    read(p2c[0], buf, 1);

    int r = txcommit();
    printf("child: txcommit -> %d\n", r);
    if (r == 0)
      print_file("child: committed", TESTFILE);
    else
      printf("child: commit failed (conflict)\n");

    close(p2c[0]);
    close(c2p[1]);
    exit(0);

  } else {
    // ── parent ─────────────────────────────────────────────────────
    close(p2c[0]);
    close(c2p[1]);

    if (txbegin() < 0) {
      printf("parent: txbegin failed\n");
      exit(1);
    }
    printf("parent: txbegin ok\n");

    int fd = open(TESTFILE, O_RDWR);
    if (fd < 0) {
      printf("parent: open failed\n");
      txabort();
      exit(1);
    }
    write_all(fd, "PARENT!!");
    close(fd);

    // wait for child to also have written
    read(c2p[0], buf, 1);
    // tell child parent is ready
    write(p2c[1], "r", 1);

    int r = txcommit();
    printf("parent: txcommit -> %d\n", r);
    if (r == 0)
      print_file("parent: committed", TESTFILE);
    else
      printf("parent: commit failed (conflict)\n");

    wait(&st);

    printf("=== final state ===\n");
    print_file("final", TESTFILE);
    printf("expected: exactly one of PARENT!! or CHILD!!!, not ORIGINAL\n");

    close(c2p[0]);
    close(p2c[1]);
    unlink(TESTFILE);
  }
}

/*static void test_asymmetric_conflict(void) {

}*/

int main(void) {
  test_commit();
  test_abort();
  test_conflict();

  // TODO: Adding more tests here later
  // test_asymmetric_conflict();

  exit(0);
}
