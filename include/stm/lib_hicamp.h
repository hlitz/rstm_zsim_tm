#include <stdlib.h>

extern "C" {
  void* hcmalloc (size_t size);
  void hcfree (void * ptr);
  void* hccalloc (size_t num, size_t size);
  void hcaddconstraint (void* src, void* dest);
  // __attribute__ ((noinline)) unsigned long long TMpromotedread(unsigned long long addr);
  __attribute__ ((noinline)) unsigned long long tm_read_promo(unsigned long long* addr);
#define TM_READ_PROMOTED(x) tm_read_promo((unsigned long long*)&x)

}


