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
#include <iostream>
#include <string>

#include <unistd.h>
#include <pthread.h>
#include <cpuid.h>

#define NDEBUG 
/* -------------------------------------------------------------------------- */
/* -- Utilities ------------------------------------------------------------- */

#define ALWAYS_INLINE_RTM __attribute__((__always_inline__)) inline
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
      int ret = _XBEGIN_STARTED;		  \
      __asm__ __volatile__(".byte 0xc7,0xf8 ; .long 0"	\
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
//static int g_locks_elided = 0;
//static int g_locks_failed = 0;
//static int g_rtm_retries  = 0;

//cacheline aligned so transactions do abort on meta conflicts/false sharing 
static uint64_t glbl_rtm_lock __attribute__((aligned(0x40))); 
static uint64_t rtm_retries[128*8] __attribute__((aligned(0x40)));

void ALWAYS_INLINE_RTM rtm_restart(){
  _xabort(0x77);
}

void ALWAYS_INLINE_RTM hle_spinlock_acquire(uint64_t* lock, uint32_t tid) EXCLUSIVE_LOCK_FUNCTION(lock){
 while (__sync_lock_test_and_set(lock, 1) != 0)
    {
      uint64_t val;
      do {
	_mm_pause();
	val = __sync_val_compare_and_swap(lock, 1, 1);
      } while (val == 1);
    }
}


void ALWAYS_INLINE_RTM
hle_spinlock_release(uint64_t* lock, uint32_t tid) UNLOCK_FUNCTION(lock)
{
  __sync_lock_release(lock);
}

static bool TM_RETRY;

void ALWAYS_INLINE_RTM
rtm_spinlock_acquire(uint64_t* lock, uint32_t tid) EXCLUSIVE_LOCK_FUNCTION(lock)
{
unsigned int tm_status = 0;
//rtm_retries[tid*8+1]=0;
 tm_try:
  if ((tm_status = _xbegin()) == _XBEGIN_STARTED) {
    /* If the lock is free, speculatively elide acquisition and continue. */
    if (*lock==0){//hle_spinlock_isfree(lock)) return;
      //rtm_retries[tid*8+1]=1;
      return;
    }
    /* Otherwise fall back to the spinlock by aborting. */
    _xabort(0xff); /* 0xff canonically denotes 'lock is taken'. */
  } 
  else {
    /* _xbegin could have had a conflict, been aborted, etc */
    if((tm_status & _XABORT_RETRY) && rtm_retries[tid*8]<4){
      //__sync_add_and_fetch(&g_rtm_retries, 1);
      //if(!TM_RETRY) rtm_retries[tid*8]++;
      _mm_pause();
      goto tm_try; // Retry 
    }
    if (tm_status & _XABORT_EXPLICIT) {
      int code = _XABORT_CODE(tm_status);
      if (code == 0xff) goto tm_fail; /* Lock was taken; fallback */
    }
  tm_fail:
    rtm_retries[tid*8] = 0x0UL;
    //    __sync_add_and_fetch(&g_locks_failed, 1);
    hle_spinlock_acquire(lock, tid);
  }
}

void ALWAYS_INLINE_RTM
rtm_glbl_spinlock_acquire(uint32_t tid){
  rtm_spinlock_acquire(&glbl_rtm_lock, tid);
}

void ALWAYS_INLINE_RTM
rtm_spinlock_release(uint64_t* lock, uint32_t tid) UNLOCK_FUNCTION(lock)
{
  /* If the lock is still free, we'll assume it was elided. This implies
     we're in a transaction. */
  if (*lock==0){// hle_spinlock_isfree(lock)) {
    //g_locks_elided += 1;
    _xend(); /* Commit transaction */
  } else {
    /* Otherwise, the lock was taken by us, so release it too. */
    hle_spinlock_release(lock, tid);
  }
}

void ALWAYS_INLINE_RTM
rtm_glbl_spinlock_release(uint32_t tid){
  //rtm_retries[tid*8+2]--;
  rtm_spinlock_release(&glbl_rtm_lock, tid);
}

 void ALWAYS_INLINE_RTM rtm_init()
 {
   if(getenv("TM_RETRY") != NULL){
     std::string str(getenv("TM_RETRY"));
     if(str.compare("FALSE")==0)
       TM_RETRY = false;
     else
       TM_RETRY = true;
   }
   else
     TM_RETRY = true;
   std::cout << "TM_RETRY env var: " << TM_RETRY << std::endl;
   //      std::cout << "glbl_lock addr " << std::hex << (uint64_t)&(glbl_rtm_lock) << std::endl;
    glbl_rtm_lock = 0;
    __sync_lock_release(&glbl_rtm_lock);

    for(int i =0; i<128;i++){
      rtm_retries[i] = 0x0UL;
    }
 }
/* -------------------------------------------------------------------------- */
/* -- Boilerplate & driver -------------------------------------------------- */


#endif // API_STAMP_RTM__
