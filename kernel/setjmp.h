#ifndef _SETJMP_H_
#define _SETJMP_H_

#include "types.h"
struct jmp_buf {
  uint64 ra;  // return address
  uint64 sp;  // stack pointer
  uint64 s0;  // callee-saved registers
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

int ksetjmp(struct jmp_buf *buf);
void klongjmp(struct jmp_buf *buf, int val);

#endif