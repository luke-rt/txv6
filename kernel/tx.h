#ifndef _TX_H_
#define _TX_H_

#include "types.h"

#define TX_INACTIVE 0
#define TX_ACTIVE 1
#define TX_ABORTED 2
#define TX_COMMITTED 3

#define MAX_WORKSET 16
#define MAX_UNDO_OPS 16

struct proc;

// Metadata for modified kernel objects specific to a transaction
struct workset_entry {
  void *header;       // pointer to stable object header (sort key)
  void *stable_data;  // original data
  void *shadow_data;  // private modified copy of data
  int read_only;  // if set to 1, only read from data, no shadow objects needed

  // type-specific operations, set when entry is created
  void (*commit_fn)(struct workset_entry *);
  void (*abort_fn)(struct workset_entry *);
  void (*lock_fn)(struct workset_entry *);
  void (*unlock_fn)(struct workset_entry *);
};

// Undo operations for modified kernel objects
struct undo_op {
  void (*fn)(void *arg);
  void *arg;
  int valid;
};

// Transaction data for a process
struct transaction {
  int status;
  int retry_count;
  uint64 start_time;

  struct trapframe *saved_tf;  // saved registers

  struct workset_entry
      workset[MAX_WORKSET];  // modified objects sorted by header addr
  int workset_size;          // num modified objects

  struct undo_op undo_ops[MAX_UNDO_OPS];  // undo operations, called on ABORT
  int n_undo_ops;                         // num undo opeartions
};

struct tx_data {
  // transaction metadata for kernel objects, ie conflict detection
};

// Kernel level allocation/free functions
struct transaction *txalloc(struct proc *p);
void txfree(struct transaction *tx);

// Transaction system calls
uint64 sys_txbegin(void);
uint64 sys_txcommit(void);
uint64 sys_txabort(void);

#endif
