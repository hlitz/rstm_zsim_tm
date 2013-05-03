#include <stdlib.h>
#include "lib_hicamp.h"

//#define INSERT_MALLOC asm("mov $666, %rcx\n\t" " movl $1027, %ecx\n\t"  "xchg %rcx, %rcx");


extern "C" {
  void* hcmalloc(size_t size){
    return malloc(size);
  }

  void hcfree(void * ptr){
    free(ptr);
  }
}
