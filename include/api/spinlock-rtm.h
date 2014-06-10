/* 
 * spinlock-rtm.c: A spinlock implementation with dynamic lock elision.
 * Copyright (c) 2013 Austin Seipp
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* -- spinlock-rtm.c contains code from tsx-tools, with this license: ------- */
/* -- https://github.com/andikleen/tsx-tools -------------------------------- */

/*
 * Copyright (c) 2012,2013 Intel Corporation
 * Author: Andi Kleen
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef API_STAMP_RTM__
#define API_STAMP_RTM__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#include <unistd.h>
#include <pthread.h>
#include <cpuid.h>

/* -------------------------------------------------------------------------- */
/* -- Utilities ------------------------------------------------------------- */

#define ALWAYS_INLINE __attribute__((__always_inline__)) inline
#define UNUSED        __attribute__((unused))
#define likely(x)     (__builtin_expect((x),1))
#define unlikely(x)   (__builtin_expect((x),0))

/* Clang compatibility for GCC */
#if !defined(__has_feature)
#define __has_feature(x) 0
#endif

#if defined(__clang__)
#define LOCKABLE            __attribute__ ((lockable))
#define SCOPED_LOCKABLE     __attribute__ ((scoped_lockable))
#define GUARDED_BY(x)       __attribute__ ((guarded_by(x)))
#define GUARDED_VAR         __attribute__ ((guarded_var))
#define PT_GUARDED_BY(x)    __attribute__ ((pt_guarded_by(x)))
#define PT_GUARDED_VAR      __attribute__ ((pt_guarded_var))
#define ACQUIRED_AFTER(...) __attribute__ ((acquired_after(__VA_ARGS__)))
#define ACQUIRED_BEFORE(...) __attribute__ ((acquired_before(__VA_ARGS__)))
#define EXCLUSIVE_LOCK_FUNCTION(...)    __attribute__ ((exclusive_lock_function(__VA_ARGS__)))
#define SHARED_LOCK_FUNCTION(...)       __attribute__ ((shared_lock_function(__VA_ARGS__)))
#define ASSERT_EXCLUSIVE_LOCK(...)      __attribute__ ((assert_exclusive_lock(__VA_ARGS__)))
#define ASSERT_SHARED_LOCK(...)         __attribute__ ((assert_shared_lock(__VA_ARGS__)))
#define EXCLUSIVE_TRYLOCK_FUNCTION(...) __attribute__ ((exclusive_trylock_function(__VA_ARGS__)))
#define SHARED_TRYLOCK_FUNCTION(...)    __attribute__ ((shared_trylock_function(__VA_ARGS__)))
#define UNLOCK_FUNCTION(...)            __attribute__ ((unlock_function(__VA_ARGS__)))
#define LOCK_RETURNED(x)    __attribute__ ((lock_returned(x)))
#define LOCKS_EXCLUDED(...) __attribute__ ((locks_excluded(__VA_ARGS__)))
#define EXCLUSIVE_LOCKS_REQUIRED(...) \
  __attribute__ ((exclusive_locks_required(__VA_ARGS__)))
#define SHARED_LOCKS_REQUIRED(...) \
  __attribute__ ((shared_locks_required(__VA_ARGS__)))
#define NO_THREAD_SAFETY_ANALYSIS  __attribute__ ((no_thread_safety_analysis))
#else
#define LOCKABLE
#define SCOPED_LOCKABLE
#define GUARDED_BY(x)
#define GUARDED_VAR
#define PT_GUARDED_BY(x)
#define PT_GUARDED_VAR
#define ACQUIRED_AFTER(...)
#define ACQUIRED_BEFORE(...)
#define EXCLUSIVE_LOCK_FUNCTION(...)
#define SHARED_LOCK_FUNCTION(...)
#define ASSERT_EXCLUSIVE_LOCK(...)
#define ASSERT_SHARED_LOCK(...)
#define EXCLUSIVE_TRYLOCK_FUNCTION(...)
#define SHARED_TRYLOCK_FUNCTION(...)
#define UNLOCK_FUNCTION(...)
#define LOCKS_RETURNED(x)
#define LOCKS_EXCLUDED(...)
#define EXCLUSIVE_LOCKS_REQUIRED(...)
#define SHARED_LOCKS_REQUIRED(...)
#define NO_THREAD_SAFETY_ANALYSIS
#endif /* defined(__clang__) */

/* -------------------------------------------------------------------------- */
/* -- HLE/RTM compatibility code for older binutils/gcc etc ----------------- */

#define PREFIX_XACQUIRE ".byte 0xF2; "
#define PREFIX_XRELEASE ".byte 0xF3; "

#define CPUID_RTM (1 << 11)
#define CPUID_HLE (1 << 4)

static inline
  int cpu_has_rtm(void)
  {
    if (__get_cpuid_max(0, NULL) >= 7) {
      unsigned a, b, c, d;
      __cpuid_count(7, 0, a, b, c, d);
      return !!(b & CPUID_RTM);
    }
    return 0;
  }

static inline UNUSED
  int cpu_has_hle(void)
{
  if (__get_cpuid_max(0, NULL) >= 7) {
    unsigned a, b, c, d;
    __cpuid_count(7, 0, a, b, c, d);
    return !!(b & CPUID_HLE);
  }
  return 0;
 }

#define _XBEGIN_STARTED         (~0u)
#define _XABORT_EXPLICIT        (1 << 0)
#define _XABORT_RETRY           (1 << 1)
#define _XABORT_CONFLICT        (1 << 2)
#define _XABORT_CAPACITY        (1 << 3)
#define _XABORT_DEBUG           (1 << 4)
#define _XABORT_NESTED          (1 << 5)
#define _XABORT_CODE(x)         (((x) >> 24) & 0xff)

#define _mm_pause()                             \
    ({                                            \
      __asm__ __volatile__("pause" ::: "memory"); \
    })                                            \

#define _xbegin()                                       \
    ({                                                    \
      int ret = /*_XBEGIN_STARTED;*/0x0			  \
      __asm__ __volatile__(".byte 0xc7,0xf8 ; .long 0"    \
	: "+a" (ret)                   \
	:                              \
      : "memory");                   \
      ret;                                                \
  })                                                    \

#define _xend()                                                 \
    ({                                                            \
      __asm__ __volatile__(".byte 0x0f,0x01,0xd5"                 \
			   ::: "memory");                         \
 })

#define _xabort(status)                                         \
    ({                                                            \
      __asm__ __volatile__( ".byte 0xc6,0xf8,%P0"                 \
	:                                     \
	: "i" (status)                        \
			    : "memory");                          \
 })

#define _xtest()                                                 \
    ({                                                             \
      unsigned char out;                                           \
      __asm__ __volatile__( ".byte 0x0f,0x01,0xd6 ; setnz %0"      \
	: "=r" (out)                           \
	:                                      \
			    : "memory");                           \
      out;                                                         \
    })

/* -------------------------------------------------------------------------- */
/* -- A simple spinlock implementation with lock elision -------------------- */

/* Statistics */
 static int g_locks_elided = 0;
 static int g_locks_failed = 0;
 static int g_rtm_retries  = 0;

 typedef struct LOCKABLE spinlock { int v; } spinlock_t;
//void (*dyn_spinlock_acquire)(spinlock_t*);
// void (*dyn_spinlock_release)(spinlock_t*);

/*void dyn_spinlock_init(spinlock_t* lock){
  lock->v = 0;
  }*/

void  hle_spinlock_acquire(spinlock_t* lock) EXCLUSIVE_LOCK_FUNCTION(lock){
  std::cou << "spin lock acquire " << std::endl;
  while (__sync_lock_test_and_set(&lock->v, 1) != 0)
    {
      int val;
      do {
	_mm_pause();
	val = __sync_val_compare_and_swap(&lock->v, 1, 1);
      } while (val == 1);
    }
}

bool
  hle_spinlock_isfree(spinlock_t* lock)
{
  return (__sync_bool_compare_and_swap(&lock->v, 0, 0) ? true : false);
}

void
  hle_spinlock_release(spinlock_t* lock) UNLOCK_FUNCTION(lock)
{
  __sync_lock_release(&lock->v);
}

void
  rtm_spinlock_acquire(spinlock_t* lock) EXCLUSIVE_LOCK_FUNCTION(lock)
{
  unsigned int tm_status = 0;

 tm_try:
  if ((tm_status = _xbegin()) == _XBEGIN_STARTED) {
    assert(0);
    /* If the lock is free, speculatively elide acquisition and continue. */
    if (hle_spinlock_isfree(lock)) return;

    /* Otherwise fall back to the spinlock by aborting. */
    _xabort(0xff); /* 0xff canonically denotes 'lock is taken'. */
  } else {
    assert(0);
    //    std::cout << "abort " << tm_status << endl;
    /* _xbegin could have had a conflict, been aborted, etc */
    if (tm_status & _XABORT_RETRY) {
      //__sync_add_and_fetch(&g_rtm_retries, 1);
      goto tm_try; /* Retry */
    }
    if (tm_status & _XABORT_EXPLICIT) {
      int code = _XABORT_CODE(tm_status);
      if (code == 0xff) goto tm_fail; /* Lock was taken; fallback */
    }

#ifndef NDEBUG
    fprintf(stderr, "TSX RTM: failure; (code %d)\n", tm_status);
#endif /* NDEBUG */
  tm_fail:
    //__sync_add_and_fetch(&g_locks_failed, 1);
    hle_spinlock_acquire(lock);
  }
}

void
  rtm_spinlock_release(spinlock_t* lock) UNLOCK_FUNCTION(lock)
{
  /* If the lock is still free, we'll assume it was elided. This implies
     we're in a transaction. */
  if (hle_spinlock_isfree(lock)) {
    //g_locks_elided += 1;
    _xend(); /* Commit transaction */
  } else {
    /* Otherwise, the lock was taken by us, so release it too. */
    hle_spinlock_release(lock);
  }
}

/* -------------------------------------------------------------------------- */
/* -- Intrusive linked list ------------------------------------------------- */
/*
#define container_of(ptr, type, member) ({                      \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);      \
    (type *)( (char *)__mptr - offsetof(type,member) ); })

 typedef struct node {
   struct node* next;
 } node_t;

 typedef struct list { node_t root; } list_t;

void
  new_list(list_t* list)
{
  list->root.next = NULL;
}

void
  push_list(list_t* list, node_t* node)
{
  node->next = list->root.next;
  list->root.next = node;
}

void
  pop_list(list_t* list)
{
  list->root.next = list->root.next;
}
/*
/* -------------------------------------------------------------------------- */
/* -- Sample threaded application #1 ---------------------------------------- */
/*
 static spinlock_t g_lock;
 static int g_testval GUARDED_BY(g_lock);
 static list_t g_list GUARDED_BY(g_lock);

 typedef struct intobj {
   unsigned int v;
   node_t node;
 } intobj_t;

 typedef struct threadctx {
   int id;
 } threadctx_t;

static void
  update_ctx(intobj_t* obj) EXCLUSIVE_LOCKS_REQUIRED(g_lock)
{
#ifndef NDEBUG
  // The following print is disabled because the synchronization otherwise
     // kills any possible TSX transactions. 
       // fprintf(stderr, "Node insert: %d:0x%p\n", obj->v, obj); 
#endif // NDEBUG 
  g_testval++;
  push_list(&g_list, &obj->node);
}

static void*
  thread(void* voidctx)
{
#define NOBJS 32
  threadctx_t* ctx = (threadctx_t*)voidctx;
  intobj_t* objs[NOBJS];

  for (int i = 0; i < NOBJS; i++) {
    objs[i] = (intobj_t*)malloc(sizeof(intobj_t));
    objs[i]->v = (i+1)*(ctx->id);
  }

  for (int i = 0; i < NOBJS; i++) {
    dyn_spinlock_acquire(&g_lock);
    update_ctx(objs[i]);
    dyn_spinlock_release(&g_lock);
  }
  return NULL;
#undef NOBJS
}

int
  begin(int nthr)
{
  // Initialize contexts 
  pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * nthr);
  threadctx_t* ctxs = (threadctx*)malloc(sizeof(threadctx_t) * nthr);
  if (threads == NULL || ctxs == NULL) {
    printf("ERROR: could not allocate thread structures!\n");
    return -1;
  }
  for (int i = 0; i < nthr; i++) ctxs[i].id = i+1;

  // Spawn threads & wait 
  new_list(&g_list);
  dyn_spinlock_init(&g_lock);
  fprintf(stderr, "Creating %d threads...\n", nthr);
  for (int i = 0; i < nthr; i++) {
    if (pthread_create(&threads[i], NULL, thread, (void*)&ctxs[i])) {
      printf("ERROR: could not create threads #%d!\n", i);
      return -1;
    }
  }
  for (int i = 0; i < nthr; i++) pthread_join(threads[i], NULL);

  // free list contents 
  int total_entries = 0;
  node_t* cur = g_list.root.next;
  while (cur != NULL) {
    intobj_t* obj = container_of(cur, intobj_t, node);
#ifndef NDEBUG
    fprintf(stderr, "Read value (%d:0x%p): %d\n", total_entries+1, obj, obj->v);
#endif 
    cur = cur->next;
    free(obj);
    total_entries++;
  }

  fprintf(stderr, "OK, done.\n");
  fprintf(stderr, "Stats:\n");
  fprintf(stderr, "  total entries:\t\t%d\n", total_entries);
  fprintf(stderr, "  g_testval:\t\t\t%d\n", g_testval);
  fprintf(stderr, "  Successful RTM elisions:\t%d\n", g_locks_elided);
  fprintf(stderr, "  Failed RTM elisions:\t\t%d\n", g_locks_failed);
  fprintf(stderr, "  RTM retries:\t\t\t%d\n", g_rtm_retries);
  free(ctxs);
  free(threads);

  return 0;
}
*/
/* -------------------------------------------------------------------------- */
/* -- Boilerplate & driver -------------------------------------------------- */

 void __attribute__((constructor))
   init()
 {
   
   /*
   int rtm = cpu_has_rtm();
#ifndef NDEBUG
   int hle = cpu_has_hle();
   printf("TSX HLE: %s\nTSX RTM: %s\n", hle ? "YES" : "NO", rtm ? "YES" : "NO");
#endif
  
   if (rtm == true) {
#if __has_feature(thread_sanitizer) || __has_feature(address_sanitizer)
    
     dyn_spinlock_acquire = &hle_spinlock_acquire;
     dyn_spinlock_release = &hle_spinlock_release;
#else
    
     dyn_spinlock_acquire = &rtm_spinlock_acquire;
     dyn_spinlock_release = &rtm_spinlock_release;
#endif
   } else {
     dyn_spinlock_acquire = &hle_spinlock_acquire;
     dyn_spinlock_release = &hle_spinlock_release;
   }

#ifndef NDEBUG
   printf("\n");
#endif
 }*/
 /*
int
  main(int ac, char **av)
{
  int nthr = 0;
  if (ac != 2) {
    printf("Usage: %s <number of threads>\n", av[0]);
    return -1;
  }
  nthr = atoi(av[1]);
  if (nthr <= 0) {
    printf("ERROR: number of threads must be greater than zero!\n");
    return -1;
  }
  return begin(nthr);
}*/

#endif // API_STAMP_RTM__
