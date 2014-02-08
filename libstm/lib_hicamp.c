#include <stdlib.h>
#include "lib_hicamp.h"
#include <stdio.h>


//#define INSERT_MALLOC asm("mov $666, %rcx\n\t" " movl $1027, %ecx\n\t"  "xchg %rcx, %rcx");

void* hcmalloc(size_t size){
  void * ptr = malloc(size);
  //  printf( "APP malloc: %Lx\n", (unsigned long long)ptr);
  return ptr;
}

void* hccalloc(size_t num, size_t size){
  void * ptr = malloc(size);
  //  printf( "APP calloc: %x\n", (unsigned long long)ptr);
  return ptr;
}

void hcfree(void * ptr){
  //printf("APP FREE %p", ptr);
  free(ptr);
}

void hcaddconstraint(void* src, void* dest){
  printf("add constraint");
}


__attribute__ ((noinline)) unsigned long long TMpromotedread(unsigned long long addr){
  printf("prmoted read this should not be shown!\n");
  return 0;
}

unsigned long long tm_read_promo(unsigned long long* addr){
  TMpromotedread((unsigned long long)addr); 
  return *addr;
} 


