#include "tx.h"
#include "kernel/defs.h"
#include "proc.h"

struct transaction *txalloc(struct proc *p) {
  struct transaction *tx = (struct transaction *)kalloc();
  tx->retry_count = 0;
  tx->start_time = 0;
  tx->status = TX_INACTIVE;

  return tx;
}

void txfree(struct transaction *tx) {
  kfree((void *)tx);
}

int txcommit(struct transaction *tx) {
  for (int i = 0; i < tx->workset_size; i++) {
    struct workset_entry *entry = &tx->workset[i];
    entry->lock_fn(&tx->workset[i]);
    entry->commit_fn(entry);
    entry->unlock_fn(&tx->workset[i]);
  }
  return 0;
}

int txabort(struct transaction *tx) {
  for (int i = 0; i < tx->workset_size; i++) {
    struct workset_entry *entry = &tx->workset[i];
    entry->lock_fn(&tx->workset[i]);
    entry->abort_fn(entry);
    entry->unlock_fn(&tx->workset[i]);
  }
  return 0;
}

uint64 sys_txbegin(void) {
  int flags;
  argint(0, &flags);  // get first argument from a0

  struct proc *p = myproc();
  p->tx->status = TX_ACTIVE;
  acquire(&tickslock);
  p->tx->start_time = ticks;
  release(&tickslock);
  p->tx->retry_count = 0;

  return 0;
}

uint64 sys_txcommit(void) {
  struct proc *p = myproc();
  if (p->tx->status != TX_ACTIVE)
    return -1;

  return txcommit(p->tx);  // returns 0 on success, -1 on conflict
}

uint64 sys_txabort(void) {
  struct proc *p = myproc();
  return txabort(p->tx);
}
