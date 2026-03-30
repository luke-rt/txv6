#ifndef _BUF_H_
#define _BUF_H_

#include "types.h"
#include "sleeplock.h"
#include "fs.h"

struct buf {
  int valid;  // has data been read from disk?
  int disk;   // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev;  // LRU cache list
  struct buf *next;

  // pointer to the transactional payload
  // on commit only this pointer is swapped
  struct buf_data *data;
};

struct buf_data {
  uchar bytes[BSIZE];
};

#endif
