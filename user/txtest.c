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

/*
* Testing transaction that is voluntarily aborted at the user level
* with the txabort() system call.
* Final file contents should not be modified by the incomplete tx.
*/
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

  printf("=== test_ww_conflict ===\n");
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

/*
* Testing a transactional process that is ended early via crash/fault.
* The TESTFILE contents should appear identical to the user both before
* and after the process "crashes" with exit(1).
*/
static void test_implicit_abort(void) {
  printf("=== test_implicit_abort ===\n");

  reset_file(TESTFILE, "ORIGINAL");

  int pid = fork();
  if (pid == 0) {
    txbegin();

    int fd = open(TESTFILE, O_RDWR);
    write_all(fd, "BROKEN!!");
    close(fd);

    // Simulate crash
    exit(1);
  }

  wait(0);

  print_file("after crash (should be ORIGINAL)", TESTFILE);
}


/*
* Testing one process in transactional mode, and one regular process.
* Both processes try to write to the same file.
* Transactional process may need to abort or take priority, depending on our policy.
*/
/*static void test_asymmetric_conflict(void) {

}*/

/*
* Testing a transaction that performs multiple writes.
* Shows operational ordering inside the tx.
*
*/
static void test_multiple_writes(void) {
  printf("=== test_multiple_writes ===\n");

  reset_file(TESTFILE, "START");

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

  print_file("final (should be BBBB or merged depending on impl)", TESTFILE);
}

/*
* Testing transaction within a transaction.
* Since we do not have support for this feature at the moment,
* the transaction should abort.
*/
static void test_nested(void) {
  printf("=== test_nested ===\n");

  if (txbegin() < 0) exit(1);

  int r = txbegin();
  printf("second txbegin -> %d (should fail)\n", r);

  txabort();
}


int main(void) {
  test_commit();
  test_abort();
  test_ww_conflict();

  // Unverified tests
  test_implicit_abort();
  test_multiple_writes();
  test_nested();

  // TODO: Adding more tests here later
  // test_asymmetric_conflict();

  exit(0);
}
