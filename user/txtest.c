// user/txtest.c
#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "user/user.h"

#include "user/test_helpers.h"

#define TESTFILE "txdemo.txt"


// ================================================
// ******************* TESTS **********************
// ================================================

/*
* Testing a transaction performing a complete txbegin()->txstatus()->txcommit()
* procedure. We print file contents throughout the execution trace to ensure
* that integrity of the data is maintained in the transactional setting.
*/
static void test_commit(void) {
  int fd;
  int r;

  printf("––– TEST_COMMIT –––\n");
  reset_file(TESTFILE, "ORIGIN");
  print_file("Initial stable", TESTFILE);

  r = txbegin();
  printf("txbegin: %d\n", r);
  if (r < 0)
    exit(1);

  fd = open(TESTFILE, O_RDWR);
  if (fd < 0) {
    printf("open(O_RDWR) failed\n");
    txabort();
    exit(1);
  }

  write_all(fd, "SHADOW");
  close(fd);

  printf("txstatus: %d\n", txstatus());
  print_file("Inside tx (shadow)", TESTFILE);
  child_print("Outside tx before commit (stable)", TESTFILE);

  r = txcommit();
  printf("txcommit: %d\n", r);
  if (r < 0)
    exit(1);

  printf("—–– Final State ———\n");
  printf("txstatus: %d\n", txstatus());
  child_print("Outside tx after commit (stable)", TESTFILE);
  print_file("After commit (stable)", TESTFILE);
  printf("\n");

  unlink(TESTFILE);
}

/*
* Testing transaction that is voluntarily aborted at the user level
* with the txabort() system call.
* Final file contents should not be modified by the incomplete tx.
*/
static void test_abort(void) {
  int fd;
  int r;

  printf("––– TEST_ABORT –––\n");
  reset_file(TESTFILE, "ORIGIN");
  print_file("Initial stable", TESTFILE);

  r = txbegin();
  printf("txbegin: %d\n", r);
  if (r < 0)
    exit(1);

  fd = open(TESTFILE, O_RDWR);
  if (fd < 0) {
    printf("open(O_RDWR) failed\n");
    txabort();
    exit(1);
  }

  write_all(fd, "SHADOW");
  close(fd);

  // Shadow write visible inside transaction
  print_file("Inside tx (shadow)", TESTFILE);
  child_print("Outside tx before abort (stable)", TESTFILE);

  r = txabort();
  printf("txabort: %d\n", r);
  if (r < 0)
    exit(1);

  // After abort, file must show original content
  printf("—–– Final State ———\n");
  printf("txstatus: %d\n", txstatus());
  child_print("Outside tx after abort (ORIGIN)", TESTFILE);
  print_file("After abort (ORIGIN)", TESTFILE);
  printf("\n");

  unlink(TESTFILE);
}

/*
* Testing two processes both in transactional mode.
* Both parent and child attempt to open a file and write to the same file.
* Conflict detection will mitigate the write-write conflict and abort
* one of the transactions.
*/
static void test_ww_conflict(void) {
  int pid;
  int st = 0;
  int p2c[2], c2p[2];
  char buf[1];

  printf("––– TEST_WW_CONFLICT –––\n");
  reset_file(TESTFILE, "ORIGIN");
  print_file("Initial stable", TESTFILE);

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
      printf("Child: txbegin failed\n");
      exit(1);
    }
    printf("Child: txbegin success\n");

    int fd = open(TESTFILE, O_RDWR);
    if (fd < 0) {
      printf("Child: open failed\n");
      txabort();
      exit(1);
    }
    write_all(fd, "CHILD");
    close(fd);

    // tell parent we are inside tx and have written
    write(c2p[1], "r", 1);
    // wait for parent to also have written
    read(p2c[0], buf, 1);

    int r = txcommit();
    printf("Child txcommit: %d\n", r);
    if (r == 0)
      print_file("Child: txcommit success", TESTFILE);
    else
      printf("Child: txcommit failed due to conflict\n");

    close(p2c[0]);
    close(c2p[1]);
    exit(0);

  } else {
    // ── parent ─────────────────────────────────────────────────────
    close(p2c[0]);
    close(c2p[1]);

    if (txbegin() < 0) {
      printf("Parent: txbegin failed\n");
      exit(1);
    }
    printf("Parent: txbegin success\n");

    int fd = open(TESTFILE, O_RDWR);
    if (fd < 0) {
      printf("Parent: open failed\n");
      txabort();
      exit(1);
    }
    write_all(fd, "PARENT");
    close(fd);

    // wait for child to also have written
    read(c2p[0], buf, 1);
    // tell child parent is ready
    write(p2c[1], "r", 1);

    int r = txcommit();
    printf("Parent txcommit: %d\n", r);
    if (r == 0)
      print_file("Parent: txcommit success", TESTFILE);
    else
      printf("Parent: commit failed (conflict)\n");

    wait(&st);

    printf("—–– Final State ———\n");
    print_file("Actual", TESTFILE);
    printf("Expected: exactly one of PARENT or CHILD, not ORIGIN\n");
    printf("\n");

    close(c2p[0]);
    close(p2c[1]);
    unlink(TESTFILE);
  }
}

/*
* Testing a transaction that performs multiple writes.
* Shows operational ordering inside the tx.
*
*/
static void test_multiple_writes(void) {
  printf("––– TEST_MULTIPLE_WRITES –––\n");

  reset_file(TESTFILE, "");

  txbegin();

  int fd = open(TESTFILE, O_RDWR);
  write_all(fd, "AAAA");
  close(fd);

  fd = open(TESTFILE, O_RDWR);
  write_all(fd, "BBBB");
  close(fd);

  fd = open(TESTFILE, O_RDWR);
  write_all(fd, "CCCC");
  close(fd);

  txcommit();

  printf("—–– Final State ———\n");
  print_file("Actual", TESTFILE);
  printf("Expected: CCCC\n");
  printf("\n");

  unlink(TESTFILE);
}

/*
* Testing transaction within a transaction.
* Would need further testing in future,
* this feature is not fully supported right now.
*/
static void test_nested(void) {
  printf("––– TEST_NESTED –––\n");

  if (txbegin() < 0) exit(1);

  int r = txbegin();
  printf("Second txbegin: %d\n", r);
  printf("\n");
  
  txabort();
  txabort();
}


int main(void) {
  test_commit();
  test_abort();
  test_multiple_writes();
  test_ww_conflict();

  if (false) {
    test_nested();
  }

  exit(0);
}
