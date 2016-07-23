#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "mem.h"

int main(int argc, char *argv[]) {
  void *space = Mem_Init(4096);
  assert(space != NULL);
  printf("Got space %p\n", space);
  int *x = Mem_Alloc(sizeof(int));
  //int *y = Mem_Alloc(sizeof(int));
  //assert(x != NULL && y != NULL);

  //printf("pre x = %p\n", x);
  //printf("pre y = %p\n", y);

  // use x and y
  *x = 123;
  //*y = 456;

  //printf("post x = %p\n", x);
  //printf("post y = %p\n", y);

  //Mem_Free(x);
  //Mem_Free(y);

  //int64_t *x;
  // should loop forever
  
  while ((x = Mem_Alloc(sizeof(int))) != NULL) {
    printf("x = %p\n", x);
    //Mem_Free(x);
    //Mem_Dump();
  }

  Mem_Dump();
  return 0;
}
