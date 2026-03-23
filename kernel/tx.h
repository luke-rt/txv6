#ifndef _TX_H_
#define _TX_H_

#include "types.h"

#define TX_INACTIVE 0
#define TX_ACTIVE 1
#define TX_ABORTED 2

struct proc;

struct transaction {
  int status;
  int retry_count;
  uint64 start_time;
};

struct transaction *txalloc(struct proc *p);
void txfree(struct transaction *tx);

#endif