#include "tx.h"
#include "kernel/defs.h"

struct transaction *txalloc(struct proc *p) {
  struct transaction *tx = (struct transaction *)kalloc();
  tx->retry_count = 0;
  tx->start_time = 0;  // TODO;
  tx->status = TX_INACTIVE;

  return tx;
}

void txfree(struct transaction *tx) {
  kfree((void *)tx);
}

uint64 sys_txbegin(void) {
  return 0;
}

uint64 sys_txcommit(void) {
  return 0;
}

uint64 sys_txabort(void) {
  return 0;
}