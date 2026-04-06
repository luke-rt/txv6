#include "tx.h"
#include "defs.h"
#include "file.h"
#include "kernel/buf.h"
#include "proc.h"

// test comment

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

// returns pointer to shadow copy of data, creating one if needed
void *txshadow(void *header, int read_only, struct tx_ops *ops) {
  struct proc *p = myproc();
  struct transaction *tx = p->tx;

  // if shadow copy already exists, return it
  for (int i = 0; i < tx->workset_size; i++) {
    if (tx->workset[i].header == header) {
      return tx->workset[i].shadow_data;
    }
  }

  // workset full, should abort transaction
  if (tx->workset_size >= MAX_WORKSET)
    return 0;

  // Create new shadow copy
  struct workset_entry *e = &tx->workset[tx->workset_size++];
  e->header = header;
  e->read_only = read_only;
  e->shadow_data = kalloc();
  if (e->shadow_data == 0)
    panic("txshadow: out of memory");

  e->ops = ops;

  // TODO: copy on write?
  memmove(e->shadow_data, *e->ops->get_data_ptr(header), ops->data_size);

  return e->shadow_data;
}

// returns data pointer of kernel object
// If called inside an active transaction, returns pointer to shadow copy of
// data
void *txdata(void *header, int read_only, struct tx_ops *ops) {
  struct proc *p = myproc();
  struct transaction *tx = p->tx;

  if (tx && tx->status == TX_ACTIVE) {
    return txshadow(header, read_only, ops);
  }

  return *ops->get_data_ptr(header);
}

void initxobj(struct tx_data *xobj) {
  xobj->writer = 0;
  xobj->reader_count = 0;
  initlock(&xobj->lock, "xobj");
}

//
// INODE specific transactions
//

static void inode_commit(struct workset_entry *e) {
  struct inode *ip = (struct inode *)e->header;
  begin_op();

  // write to disk — status is TX_COMMITTED so iupdate won't skip
  if (!e->read_only) {
    iupdate(ip);
  }

  // Decr reference count
  iput(ip);
  end_op();
}

static void inode_abort(struct workset_entry *e) {
  if (e->shadow_data)
    kfree(e->shadow_data);

  iput((struct inode *)e->header);
}

static void inode_lock(struct workset_entry *e) {
  struct inode *ip = (struct inode *)e->header;
  ilock(ip);
}

static void inode_unlock(struct workset_entry *e) {
  struct inode *ip = (struct inode *)e->header;
  iunlock(ip);
}

static struct tx_data *inode_get_xobj(void *header) {
  return &((struct inode *)header)->xobj;
}

static void **inode_get_data_ptr(void *header) {
  return (void **)&((struct inode *)header)->data;
}

static struct tx_ops inode_ops = {.data_size = sizeof(struct inode_data),
                                  .get_xobj = inode_get_xobj,
                                  .get_data_ptr = inode_get_data_ptr,
                                  .commit_fn = inode_commit,
                                  .abort_fn = inode_abort,
                                  .lock_fn = inode_lock,
                                  .unlock_fn = inode_unlock};

// Returns the appropriate inode_data for the current context:
// shadow copy if inside an active transaction, stable data otherwise
// should only be called with ilock
struct inode_data *idata(struct inode *ip) {
  return (struct inode_data *)txdata(ip, 0, &inode_ops);
}

struct inode_data *idata_ro(struct inode *ip) {
  return (struct inode_data *)txdata(ip, 1, &inode_ops);
}

//
// Kernel Buffer specific transactions
//

static void buf_commit(struct workset_entry *e) {
  struct buf *bp = (struct buf *)e->header;

  begin_op();

  acquiresleep(&bp->lock);
  log_write(bp);
  releasesleep(&bp->lock);

  end_op();

  bunpin(bp);
}
static void buf_abort(struct workset_entry *e) {
  struct buf *bp = (struct buf *)e->header;

  if (e->newly_allocated) {
    // this block was balloced inside the tx and isn't referenced by the stable inode
    // disk block needs to be freed
    begin_op();
    bfree(bp->dev, bp->blockno);
    end_op();
  }

  if (e->shadow_data)
    kfree(e->shadow_data);

  bunpin(bp);
}

static void buf_lock(struct workset_entry *e) {
  struct buf *bp = (struct buf *)e->header;
  acquiresleep(&bp->lock);
}

static void buf_unlock(struct workset_entry *e) {
  struct buf *bp = (struct buf *)e->header;
  releasesleep(&bp->lock);
}

static struct tx_data *buf_get_xobj(void *header) {
  return &((struct buf *)header)->xobj;
}

static void **buf_get_data_ptr(void *header) {
  return (void **)&((struct buf *)header)->data;
}

static struct tx_ops buffer_ops = {.data_size = sizeof(struct buf_data),
                                   .get_xobj = buf_get_xobj,
                                   .get_data_ptr = buf_get_data_ptr,
                                   .commit_fn = buf_commit,
                                   .abort_fn = buf_abort,
                                   .lock_fn = buf_lock,
                                   .unlock_fn = buf_unlock};

// Returns the appropriate buf_data for the current context:
// shadow copy if inside an active transaction, stable data otherwise
// Need to pin if new shadow created, otherwise buf might get flushed
// before transaction is completed
struct buf_data *bdata(struct buf *bp) {
  struct proc *p = myproc();
  if (p && p->tx && p->tx->status == TX_ACTIVE) {
    int old_size = p->tx->workset_size;
    struct buf_data *d = (struct buf_data *)txshadow(bp, 0, &buffer_ops);
    if (p->tx->workset_size > old_size) {
      bpin(bp);  // new shadow was created
      // bpin increases to refcnt to 1 when block is newly allocated
      p->tx->workset[p->tx->workset_size - 1].newly_allocated = (bp->refcnt == 1);
    }
    return d;
  }
  return bp->data;
}
