#include "types.h"
#include "tx.h"
#include "defs.h"
#include "proc.h"
#include "setjmp.h"

uint64 sys_txbegin(void) {
  int flags;
  argint(0, &flags);  // get first argument from a0
  (void)flags;

  struct proc *p = myproc();
  if (p->tx == 0)
    return -1;

  p->tx->status = TX_ACTIVE;
  p->tx->workset_size = 0;
  acquire(&tickslock);
  p->tx->start_time = ticks;
  release(&tickslock);
  p->tx->retry_count = 0;

  if (p->tx->saved_tf == 0)
    p->tx->saved_tf = kalloc();
  memmove(p->tx->saved_tf, p->trapframe, sizeof(struct trapframe));

  // snapshot kernel stack — for mid-syscall abort via klongjmp
  // ksetjmp returns 0 here on normal entry
  // returns 1 when klongjmp jumps back here after an abort
  if (ksetjmp(&p->tx->kjmp) != 0) {
    // we got here via klongjmp from tx_abort_now()
    // kernel stack is clean, now rewind user state
    memmove(p->trapframe, p->tx->saved_tf, sizeof(struct trapframe));
    p->trapframe->a0 = -1;  // txbegin returns -1 to user on retry
    return -1;              // this actually goes into trapframe->a0 anyway
  }

  p->tx->kjmp_valid = 1;
  return 0;
}

uint64 sys_txcommit(void) {
  struct proc *p = myproc();
  if (p->tx == 0)
    return -1;
  if (p->tx->status != TX_ACTIVE)
    return -1;

  p->tx->status = TX_COMMITTED;

  // acquires all locks, then commits all objects, then unlocks
  // TODO: handle case where cannot acquire a lock
  for (int i = 0; i < p->tx->workset_size; i++) {
    struct workset_entry *e = &p->tx->workset[i];
    if (e->ops->lock_fn)
      e->ops->lock_fn(e);
  }
  for (int i = 0; i < p->tx->workset_size; i++) {
    // copy shadow data to stable data
    struct workset_entry *e = &p->tx->workset[i];
    void **data_ptr = e->ops->get_data_ptr(e->header);
    kfree(*data_ptr);
    *data_ptr = e->shadow_data;
  }
  for (int i = 0; i < p->tx->workset_size; i++) {
    struct workset_entry *e = &p->tx->workset[i];
    if (e->ops->unlock_fn)
      e->ops->unlock_fn(e);
  }
  for (int i = 0; i < p->tx->workset_size; i++) {
    struct workset_entry *entry = &p->tx->workset[i];
    if (entry->ops->commit_fn)
      entry->ops->commit_fn(entry);
  }

  p->tx->workset_size = 0;

  return 0;
}

uint64 sys_txabort(void) {
  struct proc *p = myproc();
  if (p->tx == 0)
    return -1;
  if (p->tx->status != TX_ACTIVE)
    return -1;

  p->tx->status = TX_ABORTED;

  for (int i = 0; i < p->tx->workset_size; i++) {
    struct workset_entry *entry = &p->tx->workset[i];
    if (entry->ops->lock_fn)
      entry->ops->lock_fn(entry);
  }
  for (int i = 0; i < p->tx->workset_size; i++) {
    struct workset_entry *entry = &p->tx->workset[i];
    if (entry->ops->unlock_fn)
      entry->ops->unlock_fn(entry);
  }
  for (int i = 0; i < p->tx->workset_size; i++) {
    struct workset_entry *entry = &p->tx->workset[i];
    if (entry->ops->abort_fn)
      entry->ops->abort_fn(entry);
  }
  p->tx->workset_size = 0;

  return 0;
}

uint64 sys_txstatus(void) {
  struct proc *p = myproc();
  if (p->tx == 0)
    return -1;

  return p->tx->status;
}
