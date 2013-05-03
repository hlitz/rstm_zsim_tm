#include <stdlib.h>

extern "C" {
  void* hcmalloc (size_t size);
  void hcfree (void * ptr);
  void* hccalloc (size_t num, size_t size);
  void hcaddconstraint (void* src, void* dest);
}


