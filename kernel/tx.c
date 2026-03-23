#include "tx.h"
#include "defs.h"
#include "file.h"
#include "proc.h"

struct transaction *txalloc() {
  struct transaction *tx = (struct transaction *)kalloc();
  if (tx == 0)
    return 0;

  memset(tx, 0, sizeof(*tx));
  tx->retry_count = 0;
  tx->start_time = 0;
  tx->status = TX_INACTIVE;

  return tx;
}

void txfree(struct transaction *tx) {
  kfree((void *)tx);
}

// Must be called when already holding the lock
void inode_commit(struct workset_entry *e) {
  struct inode *ip = (struct inode *)e->header;
  struct inode_data *shadow = (struct inode_data *)e->shadow_data;

  // copy shadow data back to stable data
  memmove(&ip->data, shadow, sizeof(struct inode_data));
}

void inode_abort(struct workset_entry *e) {
  if (e->shadow_data)
    kfree(e->shadow_data);
}

void inode_lock(struct workset_entry *e) {
  struct inode *ip = (struct inode *)e->header;
  ilock(ip);
}

void inode_unlock(struct workset_entry *e) {
  struct inode *ip = (struct inode *)e->header;
  iunlock(ip);
}

// Takes an inode, gets or creates a shadow copy and workset entry
// Eventually make this generic for any kernel object header
struct inode_data *txshadow(struct inode *ip) {
  struct proc *p = myproc();
  struct transaction *tx = p->tx;

  if (tx->status != TX_ACTIVE)
    return 0;  // transaction not active

  // get existing shadow copy if exists
  for (int i = 0; i < tx->workset_size; i++) {
    if (tx->workset[i].header == ip) {
      return tx->workset[i].shadow_data;
    }
  }

  if (tx->workset_size >= MAX_WORKSET)
    return 0;  // workset full, should abort transaction

  // Create new shadow copy
  struct workset_entry *e = &tx->workset[tx->workset_size++];
  e->header = ip;
  e->read_only = 0;
  e->shadow_data = kalloc();
  if (e->shadow_data == 0)
    panic("txshadow: out of memory");
  // copy current stable data into shadow
  ilock(ip);
  e->stable_data = ip->data;
  iunlock(ip);
  memmove(e->shadow_data, e->stable_data, sizeof(struct inode_data));

  e->commit_fn = inode_commit;
  e->abort_fn = inode_abort;
  e->lock_fn = inode_lock;
  e->unlock_fn = inode_unlock;

  return e->shadow_data;
}
