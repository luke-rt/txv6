#include "types.h"
#include "tx.h"
#include "defs.h"
#include "proc.h"

uint64 sys_txbegin(void) {
  int flags;
  argint(0, &flags);  // get first argument from a0
  (void)flags;

  struct proc *p = myproc();
  if (p->tx == 0)
    return -1;

  p->tx->status = TX_ACTIVE;
  p->tx->workset_size = 0;
  p->tx->n_undo_ops = 0;
  acquire(&tickslock);
  p->tx->start_time = ticks;
  release(&tickslock);
  p->tx->retry_count = 0;

  return 0;
}

uint64 sys_txcommit(void) {
  struct proc *p = myproc();
  if (p->tx == 0)
    return -1;
  if (p->tx->status != TX_ACTIVE)
    return -1;

  for (int i = 0; i < p->tx->workset_size; i++) {
    struct workset_entry *entry = &p->tx->workset[i];
    if (entry->lock_fn)
      entry->lock_fn(entry);
    if (entry->commit_fn)
      entry->commit_fn(entry);
    if (entry->unlock_fn)
      entry->unlock_fn(entry);
  }
  p->tx->status = TX_COMMITTED;
  p->tx->workset_size = 0;
  p->tx->n_undo_ops = 0;
  return 0;
}

uint64 sys_txabort(void) {
  struct proc *p = myproc();
  if (p->tx == 0)
    return -1;
  if (p->tx->status != TX_ACTIVE)
    return -1;

  for (int i = 0; i < p->tx->workset_size; i++) {
    struct workset_entry *entry = &p->tx->workset[i];
    if (entry->lock_fn)
      entry->lock_fn(entry);
    if (entry->abort_fn)
      entry->abort_fn(entry);
    if (entry->unlock_fn)
      entry->unlock_fn(entry);
  }
  p->tx->status = TX_ABORTED;
  p->tx->workset_size = 0;
  p->tx->n_undo_ops = 0;
  return 0;
}
