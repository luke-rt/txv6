// user/txtest.c
#include "kernel/types.h"
#include "user/user.h"

int main(void) {
  int r = txbegin();
  printf("txbegin returned %d\n", r);
  r = txcommit();
  printf("txcommit returned %d\n", r);
  exit(0);
}