#ifndef MVCC_ALLOCATOR_H
#define MVCC_ALLOCATOR_H

#include "cstddef"
//#include "sit_malloc.h"
//#include "scoped_allocator"
#include "limits"

//define sit_allocator implementing requirements for an Allocator
template <class T>
class mvcc_allocator
{
public:
  //These are a little silly, no?
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;
  typedef T*        pointer;
  typedef const T*  const_pointer;
  typedef T&        reference;
  typedef const T&  const_reference;
  typedef T         value_type;
  //propagate_on_container_move_assignment??

  //Syntax for rebind is standardized - including word "other"
  template <class U>
  struct rebind { typedef mvcc_allocator<U> other; };
  
  mvcc_allocator(/*mvcc_malloc* */);
  
  template <class U>
  mvcc_allocator(const mvcc_allocator<U>&);
  //assignment version of above
  template <class U>
  mvcc_allocator<T>& operator=(const mvcc_allocator<U>&);
  
  pointer allocate(size_type n, const void* hint=NULL);
  void deallocate(void* p, size_type);
  pointer address(reference x) const {return &x; };
  const_pointer address(const_reference x) const {return &x; };
  template <class U, class... Args>
  void construct (U* p, Args&&... args){
    ::new((void *)p) U(std::forward<Args>(args)...);
  }
  template <class U>
  void destroy(U* p){
    p->~U();
  }

  size_type max_size() const{
    return std::numeric_limits<size_type>::max();
  }

  //mvcc_malloc* malloc_seg;

};

template <class V, class W>
bool operator==(const mvcc_allocator<V>& lhs, const mvcc_allocator<W>& rhs){
  return true;//lhs.malloc_seg == rhs.malloc_seg;
}

template <class V, class W>
bool operator!=(const mvcc_allocator<V>& lhs, const mvcc_allocator<W>& rhs){
  return false;//lhs.malloc_seg != rhs.malloc_seg;
}

#define MVCC_ALLOCATOR_FUNC(RETURN_T) template <class T> RETURN_T mvcc_allocator<T>
#define MVCC_ALLOCATOR_FUNC2(ADD_TEMP, RETURN_T) template <class T> ADD_TEMP RETURN_T mvcc_allocator<T>

MVCC_ALLOCATOR_FUNC()
:: mvcc_allocator (/*mvcc_malloc* _malloc_seg*/){
  //malloc_seg = _malloc_seg;
  }

MVCC_ALLOCATOR_FUNC2(template<class U>, )
:: mvcc_allocator (const mvcc_allocator<U>& tocopy){
  //malloc_seg = tocopy.malloc_seg;
}

MVCC_ALLOCATOR_FUNC2(template<class U>, mvcc_allocator<T>&)
:: operator= (const mvcc_allocator<U>& tocopy){
  //malloc_seg = tocopy.malloc_seg;
  return *this;
}

MVCC_ALLOCATOR_FUNC(T*)
:: allocate(size_type n, const void* hint){
  return (pointer)hcmalloc(n*sizeof(value_type));
  /*  pointer toRet = NULL;

  malloc_seg->update();
  int retries;
  for(retries = 0; ; retries++){
    toRet = (pointer) malloc_seg->malloc(n * sizeof(value_type));
    int commit_result = malloc_seg->commit();
    if (commit_result == -EAGAIN){
      continue;
    } else if (commit_result == 0){
      break; //success
    } else {
      printf("ERROR unrecognized error code in mvcc_allocate::allocate\n");
      exit(1);
    }
  }

  printf("Allocated %Ld bytes!\n", n*sizeof(value_type));

  return toRet;
  */
}

MVCC_ALLOCATOR_FUNC(void)
:: deallocate(void* p, size_type){
  hcfree(p);
  /*
  malloc_seg->update();
  malloc_seg->free(p);
  int commit_result = malloc_seg->commit();
  //Because of how free works, we never care about the return value of
  //the commit.
  */
}

#undef MVCC_ALLOCATOR_FUNC
#undef MVCC_ALLOCATOR_FUNC2

#endif
