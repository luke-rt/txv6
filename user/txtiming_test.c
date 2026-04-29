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

#define MAX_TRIALS 100
#define BATCH_TRIALS 10

// NUM_WRITE_OPT
// Toggle this macro between the following values 
// to adjust the volume of writes used for the batch tests
// 1 -- 256
// 2 -- 1024
// 3 -- 4096
#define NUM_WRITE_OPT 3

// WRITE_SIZE_OPT
// Toggle this macro between the following values
// to adjust the size (in bytes) of each write used for the batch tests
// 1 -- 16
// 2 -- 32
// 3 -- 64
#define WRITE_SIZE_OPT 1

// ––––––––––––––– Tx vs. no-Tx timing tests –––––––––––––––
static void test_tx_vs_notx(void) {
  int fd;
  int r;

  printf("=== tx_vs_notx ===\n");
  reset_file(TESTFILE, "");

  int num_fwrites[9] = {4, 8, 16, 32, 64, 128, 256, 512, 1024};
  int wr_size = 16;

  char *buf = malloc(wr_size);
  if (buf == 0) {
    printf("malloc failed\n");
    exit(1);
  }
  memset(buf, 'A', wr_size);

  /*printf("---- Non-transactional mode ----\n");
  for (int i = 0; i < 9; i++) {
    int fw = num_fwrites[i];

    int running_time_sum = 0;

    for (int j = 0; j < MAX_TRIALS; j++) {
      //printf("herwe!\n");
      //fprintf(2, "DEBUG: Trial %d executing right now\n", j);

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
      if (j % 10 == 0)
        reset_file(TESTFILE, "");

      running_time_sum += (end - start);
    }

    // Calculate average time per trial for writing num_fwrites[i] times
    int avg_time_elapsed = running_time_sum / MAX_TRIALS;
    fprintf(2, "NoTX -- Avg. time elapsed for %d writes across %d trials: %d ms\n", fw, MAX_TRIALS, avg_time_elapsed);
  }
  printf("--------------------------------\n");
  printf("\n");*/

  printf("------ Transactional mode ------\n");
  for (int i = 0; i < 9; i++) {
    int fw = num_fwrites[i];

    int running_time_sum = 0;

    for (int j = 0; j < MAX_TRIALS; j++) {
      fd = open(TESTFILE, O_RDWR);
      if (fd < 0) {
        printf("open(O_RDWR) failed\n");
        exit(1);
      }
      //printf("herwe!\n");
      //fprintf(2, "DEBUG: Trial %d executing right now\n", j);

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
      if (j % 10 == 0)
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
  int fd;
  int r;

  printf("=== test_optimal_batch ===\n");
  
  reset_file(TESTFILE, "");

  int total_writes = NUM_WRITE_OPT == 1 ? 256 : (NUM_WRITE_OPT == 2 ? 1024 : (NUM_WRITE_OPT == 3 ? 4096 : 256));
  int batch_sizes[8] = {1, 2, 4, 8, 16, 32, 64, 128};
  int wr_size = WRITE_SIZE_OPT == 1 ? 16 : (WRITE_SIZE_OPT == 2 ? 32 : (WRITE_SIZE_OPT == 3 ? 64 : 16));

  // Initialize the buffer
  char *buf = malloc(wr_size);
  if (buf == 0) {
    printf("malloc failed\n");
    exit(1);
  }
  memset(buf, 'B', wr_size);

  // Loop through candidate batch sizes
  for (int i = 0; i < 8; i++) {

    int running_time_sum = 0;

    for (int j = 0; j < BATCH_TRIALS; j++) {
      fprintf(2, "DEBUG: Here! i = %d, Trial #%d\n", i, j);
      int start = uptime();

      for (int k = 0; k < total_writes / batch_sizes[i]; k++) {
        // begin tx
        r = txbegin();
        if (r < 0)
          exit(1);

        fd = open(TESTFILE, O_RDWR);
        if (fd < 0) {
          printf("open(O_RDWR) failed\n");
          exit(1);
        }
        
        for (int l = 0; l < batch_sizes[i]; l++) {
          if (write(fd, buf, wr_size) != wr_size) {
            printf("write failed\n");
            exit(1);
          }
        }

        close(fd);

        r = txcommit(); // COMMIT TX
        if (r < 0)
          exit(1);
        // commit tx
      }

      int end = uptime();
      running_time_sum += (end - start);
    }

    reset_file(TESTFILE, "");

    int avg_time_elapsed = running_time_sum / BATCH_TRIALS;
    fprintf(2, "TX -- Time elapsed for %d total writes, batch size of %d, avg over %d trials: %d ms\n", 
      total_writes, 
      batch_sizes[i], 
      BATCH_TRIALS, 
      avg_time_elapsed
    );
  }

  free(buf);
}

static void test_optimal_batch_notx(void) {
  int fd;

  printf("=== test_optimal_batch_notx ===\n");
  
  reset_file(TESTFILE, "");

  int total_writes = NUM_WRITE_OPT == 1 ? 256 : (NUM_WRITE_OPT == 2 ? 1024 : (NUM_WRITE_OPT == 3 ? 4096 : 256));
  int batch_sizes[8] = {1, 2, 4, 8, 16, 32, 64, 128};
  int wr_size = WRITE_SIZE_OPT == 1 ? 16 : (WRITE_SIZE_OPT == 2 ? 32 : (WRITE_SIZE_OPT == 3 ? 64 : 16));

  // Initialize the buffer
  char *buf = malloc(wr_size);
  if (buf == 0) {
    printf("malloc failed\n");
    exit(1);
  }
  memset(buf, 'B', wr_size);

  // Loop through candidate batch sizes
  for (int i = 0; i < 8; i++) {

    int running_time_sum = 0;

    for (int j = 0; j < BATCH_TRIALS; j++) {
      fprintf(2, "DEBUG: Here! i = %d, Trial #%d\n", i, j);
      int start = uptime();

      for (int k = 0; k < total_writes / batch_sizes[i]; k++) {
        fd = open(TESTFILE, O_RDWR);
        if (fd < 0) {
          printf("open(O_RDWR) failed\n");
          exit(1);
        }
        
        for (int l = 0; l < batch_sizes[i]; l++) {
          if (write(fd, buf, wr_size) != wr_size) {
            printf("write failed\n");
            exit(1);
          }
        }

        close(fd);
      }

      int end = uptime();
      running_time_sum += (end - start);
    }

    reset_file(TESTFILE, "");

    int avg_time_elapsed = running_time_sum / BATCH_TRIALS;
    fprintf(2, "No TX -- Time elapsed for %d total writes, batch size of %d, avg over %d trials: %d ms\n", 
      total_writes, 
      batch_sizes[i], 
      BATCH_TRIALS, 
      avg_time_elapsed
    );
  }

  free(buf);
}

// –––––––––––––––– Latency tests –––––––––––––––––
static void test_tx_tail_latency(void) {
  int fd;
  int r;

  printf("=== test_tail_latency ===\n");
  
  reset_file(TESTFILE, "");
  
  // Fixing batch_size 
  int batch_size = 256;
  int wr_size = WRITE_SIZE_OPT == 1 ? 16 : (WRITE_SIZE_OPT == 2 ? 32 : (WRITE_SIZE_OPT == 3 ? 64 : 16));

  int ntrials_mult = 1;

  // Manually override wr_size for testing
  //wr_size = 256;

  // Initialize the buffer
  char *buf = malloc(wr_size);
  if (buf == 0) {
    printf("malloc failed\n");
    exit(1);
  }
  memset(buf, 'C', wr_size);

  // Keep track of array of latencies
  int *latencies = (int*) malloc((MAX_TRIALS * ntrials_mult) * sizeof(int));

  for (int i = 0; i < (MAX_TRIALS * ntrials_mult); i++) {
    int start = uptime();
    // begin tx
    r = txbegin();
    if (r < 0)
      exit(1);

    fd = open(TESTFILE, O_RDWR);
    if (fd < 0) {
      printf("open(O_RDWR) failed\n");
      exit(1);
    }

    for (int j = 0; j < batch_size; j++) {
      /*fd = open(TESTFILE, O_RDWR);
      if (fd < 0) {
        printf("open(O_RDWR) failed\n");
        exit(1);
      }*/

      if (write(fd, buf, wr_size) != wr_size) {
        printf("write failed\n");
        exit(1);
      }

      //close(fd);
    }

    close(fd);

    r = txcommit();
    if (r < 0)
      exit(1);
    // commit tx

    int end = uptime();

    // Record the latency of this batch of writes
    latencies[i] = (end - start);
    fprintf(2, "DEBUG: Latency observed for i = %d, batch size of %d: %d ms\n", i, batch_size, latencies[i]);
  }

  // Analyze our distribution of latencies
  sort(latencies, (MAX_TRIALS * ntrials_mult));

  int p50 = latencies[(MAX_TRIALS * ntrials_mult) * 50 / 100];
  int p90 = latencies[(MAX_TRIALS * ntrials_mult) * 90 / 100];
  int p99 = latencies[(MAX_TRIALS * ntrials_mult) * 99 / 100];
  int min = latencies[0];
  int max = latencies[(MAX_TRIALS * ntrials_mult) - 1];

  // Print percentiles
  printf("Latency stats (batch=%d):\n", batch_size);
  printf("min: %d ms\n", min);
  printf("p50: %d ms\n", p50);
  printf("p90: %d ms\n", p90);
  printf("p99: %d ms\n", p99);
  printf("max: %d ms\n", max);

  // Histogram-style
  // Categories/buckets: 0-3 ms, 3-10 ms, 10+ ms
  int b0_3 = 0;
  int b3_10 = 0;
  int b10_plus = 0;

  // Classify each trial
  for (int i = 0; i < (MAX_TRIALS * ntrials_mult); i++) {
    int t = latencies[i];

    if (t <= 3)
      b0_3++;
    else if (t <= 10)
      b3_10++;
    else
      b10_plus++;
  }

  // Print distribution
  printf("distribution:\n");
  printf("0–3ms:   %d\n", b0_3);
  printf("3–10ms:  %d\n", b3_10);
  printf("10+ ms:  %d\n", b10_plus);

  printf("——————————————————————————————————\n");

  free(buf);
  free(latencies);
}

int main(void) {
  // NOTE: May be helpful to comment out certain tests just to isolate
  // the one that you want to run in isolation, because each one may take
  // a while.

  // Or wrap them in if(false) conditionals

  // NOTE: Times are mistakenly reported in ms, but are actually measured in clock ticks.

  if (false)
    test_tx_tail_latency();

  test_optimal_batch();
  test_optimal_batch_notx();

  if (false)
    test_tx_vs_notx();

  exit(0);
}
