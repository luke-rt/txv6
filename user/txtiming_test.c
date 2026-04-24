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

#define MAX_TRIALS 1000

// ––––––––––––––– Tx vs. no-Tx timing tests –––––––––––––––
static void test_tx_vs_notx(void) {
  int fd;
  int r;

  printf("=== tx_vs_notx ===\n");
  reset_file(TESTFILE, "");

  int num_fwrites[12] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048};
  int wr_size = 64;

  char *buf = malloc(wr_size);
  if (buf == 0) {
    printf("malloc failed\n");
    exit(1);
  }
  memset(buf, 'A', wr_size);

  printf("---- Non-transactional mode ----\n");
  for (int i = 0; i < 12; i++) {
    int fw = num_fwrites[i];

    int running_time_sum = 0;

    for (int j = 0; j < MAX_TRIALS; j++) {
      fd = open(TESTFILE, O_RDWR);
      if (fd < 0) {
        printf("open(O_RDWR) failed\n");
        exit(1);
      }

      int start = uptime(); // Keep track of starting time

      for (int k = 0; k < fw; k++) {
        if (write(fd, buf, wr_size) != wr_size) {
          printf("write failed\n");
          exit(1);
        }
      }
      close(fd);

      int end = uptime(); // Keep track of ending time
      reset_file(TESTFILE, "");

      running_time_sum += (end - start);
    }

    // Calculate average time per trial for writing num_fwrites[i] times
    int avg_time_elapsed = running_time_sum / MAX_TRIALS;
    fprintf(2, "NoTX -- Avg. time elapsed for %d writes across %d trials: %d ms\n", fw, MAX_TRIALS, avg_time_elapsed);
  }
  printf("--------------------------------\n");
  printf("\n");

  printf("------ Transactional mode ------\n");
  for (int i = 0; i < 12; i++) {
    int fw = num_fwrites[i];

    int running_time_sum = 0;

    for (int j = 0; j < MAX_TRIALS; j++) {
      fd = open(TESTFILE, O_RDWR);
      if (fd < 0) {
        printf("open(O_RDWR) failed\n");
        exit(1);
      }
      printf("herwe!\n");
      fprintf(2, "DEBUG: Trial %d executing right now\n", j);

      int start = uptime(); // Keep track of starting time

      r = txbegin(); // START TX
      if (r < 0)
        exit(1);

      for (int k = 0; k < fw; k++) {
        if (write(fd, buf, wr_size) != wr_size) {
          printf("write failed\n");
          exit(1);
        }
      }
      close(fd);

      r = txcommit(); // COMMIT TX
      if (r < 0)
        exit(1);

      int end = uptime(); // Keep track of ending time
      reset_file(TESTFILE, "");

      running_time_sum += (end - start);
    }

    // Calculate average time per trial for writing num_fwrites[i] times
    int avg_time_elapsed = running_time_sum / MAX_TRIALS;
    fprintf(2, "TX -- Avg. time elapsed for %d writes across %d trials: %d ms\n", fw, MAX_TRIALS, avg_time_elapsed);
  }
  printf("--------------------------------\n");

  free(buf);
}


// –––––––––––––––– Write batch size tests –––––––––––––––––
static void test_optimal_batch(void) {
  printf("=== test_optimal_batch ===\n");
  printf("Does nothing! (for now)\n");

}


int main(void) {
  test_tx_vs_notx();
  test_optimal_batch();

  exit(0);
}
