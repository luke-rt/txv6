#ifndef _TX_H_
#define _TX_H_

#include "types.h"
#include "spinlock.h"
#include "setjmp.h"

#define TX_INACTIVE 0
#define TX_ACTIVE 1
#define TX_ABORTED 2
#define TX_COMMITTED 3

#define MAX_WORKSET 16
#define MAX_UNDO 16

// Metadata for modified kernel objects specific to a transaction
struct workset_entry {
  void *header;  // pointer to stable object header (sort key)
  // void *stable_data;  // original data
  void *shadow_data;  // private modified copy of data
  int read_only;  // if set to 1, only read from data, no shadow objects needed
  int newly_allocated;  // buf was balloc'd inside this tx; bfree on abort

  struct tx_ops *ops;  // operations for this object type
};

// Kernel object type specific metadata/operations
struct tx_ops {
  int data_size;  // size of data for this object
  struct tx_data *(*get_xobj)(void *header);
  void **(*get_data_ptr)(void *header);
  void (*commit_fn)(struct workset_entry *);
  void (*abort_fn)(struct workset_entry *);
  void (*lock_fn)(struct workset_entry *);
  void (*unlock_fn)(struct workset_entry *);
};

// Transaction data for a process
struct transaction {
  int status;
  int retry_count;
  uint64 start_time;

  struct trapframe *saved_tf;  // saved registers
  struct jmp_buf kjmp;         // kernel stack snapshot (for mid-syscall abort)
  int kjmp_valid;  // set to 1 if kjmp contains valid jump buffer, 0 otherwise

  struct workset_entry
      workset[MAX_WORKSET];  // modified objects sorted by header addr
  int workset_size;          // num modified objects

  void (*undo_ops[MAX_UNDO])(
      void *header);  // TODO: undo ops for in-progress transactions, to be run
                      // on abort. For example, iunlock, release buf locks, etc.
  int undo_size;
};

struct tx_data {
  struct spinlock lock;
  struct transaction *writer;
  int reader_count;
};

#endif
