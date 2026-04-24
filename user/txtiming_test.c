// user/txtiming_test.c

#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "user/user.h"

#include "user/test_helpers.h"

#define TESTFILE "txdemo.txt"

/*
* In this test file, we want to measure the overhead that transactions
* may incur on file system I/O operations. In particular, we are interested
* in exploring the timing behavior of an increasing volume of file writes.
*
* We are also interested in investigating the optimal "write batch" size for
* txcommits. We present a series of tests to measure the timing differences
* between batch sizes of 1, 2, 4, 8 ... etc.
*/

// ––––––––––––––– Tx vs. no-Tx timing tests –––––––––––––––



// –––––––––––––––– Write batch size tests –––––––––––––––––


int main(void) {

  exit(0);
}
