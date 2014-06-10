/**
 *  Copyright (C) 2013
 *  Heiner Litz, Stanford University
 */

/**
 * Hardware TM Implementation using ZSIM, we only define entry points for
 * the simulator, plus some debugging and stack tracing stuff that is 
 * disabled per default 
 */

#include "../profiling.hpp"
#include "algs.hpp"
#include "RedoRAWUtils.hpp"
#include <iostream>
#include <vector>
#include <execinfo.h>
#include <map>
#include <set>
#include <string>

using namespace std;
using stm::TxThread;
/*using stm::timestamp;
using stm::timestamp_max;
using stm::WriteSet;
using stm::OrecList;
using stm::UNRECOVERABLE;
using stm::WriteSetEntry;
using stm::orec_t;
using stm::get_orec;
*/

//Some debugging facilities
const bool DEBUG_BACKTRACE = false;//true;
const bool BENCH = true;
const bool MVCC = false;

//#define STACKTRACE asm(" movl $1028, %ecx\n\t"  "xchg %rcx, %rcx")
#define XBEGIN asm(" movl $1040, %ecx\n\t"  "xchg %rcx, %rcx")
#define XEND asm(" movl $1041, %ecx\n\t"  "xchg %rcx, %rcx")
#define XWRITE asm(" movl $1042, %ecx\n\t"  "xchg %rcx, %rcx")

const std::string filename = "rbtree";     

/* Obtain a backtrace and print it to stdout. */
void print_bt(uint64_t addr)
{
  void *array[100];
  size_t size;
  char **strings;    
  size = backtrace (array, 100);
  strings = backtrace_symbols (array, size);
  std::cout << " ------------------ addr " << addr << std::endl;
  for(uint32_t i =0; i< size; ++i){
    std::cout << strings[i] << std::endl;
  }
  std::cout << " ------------------"<<std::endl;
  free(strings);
}

/* dummy function to be replaced by hardware/zsim*/
__attribute__ ((noinline)) bool TMaddwset(uint64_t addr, uint64_t data, uint64_t codeline) {
      std::cout << "Calling dummy function TMaddwset, this text should not be shown, check pin instrumentation" << addr << std::endl; 
      return false;
}

__attribute__ ((noinline)) bool TMaddrset(uint64_t addr, uint64_t datap, uint64_t codeline) {
      std::cout << "Calling dummy function TMaddrset, this text should not be shown, check pin instrumentation" << addr << std::endl; 
      assert(0);
      return false;
}

__attribute__ ((noinline)) uint64_t TMstart() {
  std::cout << "Calling dummy function TMstart, this text should not be shown, check pin instrumentation" << std::endl; 
  return 0;
}

__attribute__ ((noinline)) bool TMcommit() {
   std::cout << "Calling dummy function TMcommit, this text should not be shown, check pin instrumentation" << std::endl; 
  return false;
}

__attribute__ ((noinline)) bool TMrocommit() {
  std::cout << "Calling dummy function TMrocommit, this text should not be shown, check pin instrumentation" << std::endl;
  return false;
}

__attribute__ ((noinline)) void TMpromotedread(uint64_t addr) {
  std::cout << "Calling dummy function promoted read, this text should not be shown, check pin instrumentation" << std::endl;
}

/**
 *  Declare the functions that we're going to implement, so that we can avoid
 *  circular dependencies.
 */
namespace {
  struct HTM
  {
      static TM_FASTCALL bool begin(TxThread*);
      static TM_FASTCALL void* read_ro(STM_READ_SIG(,,));
      static TM_FASTCALL void* read_rw(STM_READ_SIG(,,));
      static TM_FASTCALL void* read_ro_promo(STM_READ_SIG(,,));
      static TM_FASTCALL void* read_rw_promo(STM_READ_SIG(,,));
      static TM_FASTCALL void write_ro(STM_WRITE_SIG(,,,));
      static TM_FASTCALL void write_rw(STM_WRITE_SIG(,,,));
    static TM_FASTCALL void commit_ro(TxThread*);
      static TM_FASTCALL void commit_rw(TxThread*);

      static stm::scope_t* rollback(STM_ROLLBACK_SIG(,,));
      static bool irrevoc(TxThread*);
      static void onSwitchTo();
      static NOINLINE void validate(TxThread*);
  };

  /* Fine grained exponential backoff in Software */
inline uint64_t rdtsc()
{
    uint32_t lo, hi;
    __asm__ __volatile__ (
      "xorl %%eax, %%eax\n"
      "cpuid\n"
      "rdtsc\n"
      : "=a" (lo), "=d" (hi)
      :
      : "%ebx", "%ecx" );
    return (uint64_t)hi << 32 | lo;
}

  /**
   *  HTM begin:
   */
  bool
  HTM::begin(TxThread* tx)
  {
    printf("start TRX %i\n", tx->id);
    tx->txn++;
    uint64_t begintime, endtime;
    tx->allocator.onTxBegin();
    // std::cout << "Begin in SW" << std::endl;
    __sync_synchronize();
    XBEGIN;
    //    std::cout << "Begin in SW" << std::endl;
   //__sync_synchronize();
      
    /*    uint64_t wait = TMstart();
    if(wait>1){
      begintime = rdtsc();
      endtime = begintime;
      while(begintime+wait>endtime && begintime+wait>begintime){
	endtime = rdtsc();
      }
      } */
    OnFirstWrite(tx, read_rw, read_rw_promo, write_rw, commit_rw);
    return false;
  }

  /**
   *  HTM commit (read-only):
   */
  void
  HTM::commit_ro(TxThread* tx)
  {
    //printf("committing %i \n", tx->id);// << std::endl;
    /*   if(!TMrocommit()){
      //std::cout << "comit ro abort" << std::endl;
      tx->allocator.onTxAbort(); 
      tx->tmabort(tx);
      }*/
    __sync_synchronize();
    XEND;
    __sync_synchronize();
    //std::cout << "comit ro" << std::endl;

    tx->allocator.onTxCommit();
    OnReadOnlyCommit(tx);
  }

  /**
   *  HTM commit (writing context):
   *
   */
  void
  HTM::commit_rw(TxThread* tx)
  {
    //printf("committing rw %i\n", tx->id);//  << std::endl;
    /*bool res = TMcommit(); 
    if(!res) { 
      //std::cout << "comit rw abort" << std::endl;
      tx->allocator.onTxAbort(); 
      tx->tmabort(tx);
      }*/
    __sync_synchronize();
    XEND;
    __sync_synchronize();
    //std::cout << "comit rw" << std::endl;
    tx->allocator.onTxCommit(); 
    OnReadWriteCommit(tx, read_rw, read_rw_promo, write_rw, commit_rw);
  }

  /**
   *  HTM read (read-only transaction)
   * For promoted reads, SI-TM only
   */
  void*
  HTM::read_ro_promo(STM_READ_SIG(tx,addr,))
  {/*
    printf("read Ro promo %i \n", tx->id);//  << std::endl;
    uint64_t data = 0;
    TMpromotedread((uint64_t)addr);
    bool res = TMaddrset((uint64_t)addr, (uint64_t)&data, 0);
    if(!res) { 
      tx->tmabort(tx);
    }
    return (void*)data;*/
    __sync_synchronize();
    return (void*)*addr;
  }

  void* __attribute__ ((noinline))
  HTM::read_ro(STM_READ_SIG(tx,addr,))
  {

    __sync_synchronize();
    return (void*)*addr;
    //printf("read RO %i \n", tx->id);//  << std::endl;
    uint64_t data;
    uint64_t codeline = 0;
    void *array[4];
    if(DEBUG_BACKTRACE){
      print_bt((uint64_t)addr);
      backtrace(array, 4);
      if(BENCH)
	codeline = (uint64_t) array[2];
      else
	codeline = (uint64_t) array[3];	
      if(codeline == 0){
	print_bt((uint64_t)addr);
	assert(0);
      }
    }
    bool res = TMaddrset((uint64_t)addr, (uint64_t)&data, codeline);
    if(!res) {
      //debug stuff
      if(MVCC){
	print_bt((uint64_t)addr);
	assert(0);
      }
      tx->allocator.onTxAbort();
      tx->tmabort(tx);
    }
    return (void*)data;
  }

  /**
   *  HTM read (writing transaction)
   */
  void* //__attribute__ ((noinline))
  HTM::read_rw_promo(STM_READ_SIG(tx,addr,mask))
  {

    __sync_synchronize();
   return (void*)*addr;
    printf("read promo %i",  tx->id);// << std::endl;
    uint64_t data = 0;
    TMpromotedread((uint64_t)addr);
    bool res = TMaddrset((uint64_t)addr, (uint64_t)&data, 0);
    if(!res) { 
      tx->allocator.onTxAbort(); 
      tx->tmabort(tx);
    }
    return (void*)data;//read_rw(tx,addr);
  }
  
  void* __attribute__ ((noinline))
  HTM::read_rw(STM_READ_SIG(tx,addr,mask))
  {
    __sync_synchronize();
    return (void*)*addr;
    printf("read RW, %i\n", tx->id);//  << std::endl;
    uint64_t data;
    uint64_t codeline = 0;
    void *array[4];
    if(DEBUG_BACKTRACE){
      print_bt((uint64_t)addr);
      backtrace(array, 4);
     if(BENCH)
       codeline = (uint64_t) array[2];
     else
       codeline = (uint64_t) array[3];
     if(codeline == 0){
       print_bt((uint64_t)addr);
       assert(0);
     }
    }
    bool res = TMaddrset((uint64_t)addr, (uint64_t)&data, codeline);
    if(!res) { 
      tx->allocator.onTxAbort(); 
      if(MVCC){
	print_bt((uint64_t)addr);
	assert(0);
      }
      tx->tmabort(tx);
    }
    printf("END read RW, %i\n", tx->id);//  << std::endl;
    return (void*)data;
  }


  /**
   *  HTM write (read-only context)
   */
  void
  HTM::write_ro(STM_WRITE_SIG(tx,addr,val,mask))
  {
    uint64_t codeline =1;
    bool res;
    //res = TMaddwset((uint64_t)addr, (uint64_t)val, codeline);
    //XWRITE;
    __sync_synchronize();
    *addr = val;
    __sync_synchronize();
    return;
    printf("write ro, %i\n", tx->id);// << std::endl;
    codeline = 0;
    res = TMaddwset((uint64_t)addr, (uint64_t)val, codeline);
    if(!res) { 
      tx->allocator.onTxAbort(); 
      if(MVCC){
	print_bt((uint64_t)addr);
	assert(0);
      }
      tx->tmabort(tx);
    }
    OnFirstWrite(tx, read_rw, read_rw_promo, write_rw, commit_rw);
  }
    
  /**
   *  HTM write (writing context)
   */
  void
  HTM::write_rw(STM_WRITE_SIG(tx,addr,val,mask))
  {   
    uint64_t codeline =1;
    //XWRITE;
    bool res;
    //res= TMaddwset((uint64_t)addr, (uint64_t)val, codeline);
    __sync_synchronize();
    *addr = val;
    __sync_synchronize();
    return;
    printf("write RW, %i\n", tx->id);// <<std::endl;
    codeline = 0;
    res = TMaddwset((uint64_t)addr, (uint64_t)val, codeline);
    if(!res) { 
      tx->allocator.onTxAbort(); 
      if(MVCC){
	print_bt((uint64_t)addr);
	assert(0);
      }
     tx->tmabort(tx);
    }
    printf("end Write RW, %i\n", tx->id);//  << std::endl;

  }

  /**
   *  HTM unwinder:
   */
  stm::scope_t*
  HTM::rollback(STM_ROLLBACK_SIG(tx, except, len))
  {
      PreRollback(tx); 
      return PostRollback(tx, read_ro, write_ro, commit_ro);
  }

  /**
   *  HTM in-flight irrevocability:
   */
  bool
  HTM::irrevoc(TxThread*)
  {
      return false;
  }

  /**
   *  HTM validation
   */
  void
  HTM::validate(TxThread* tx)
  {
    std::cout << "validate!"<< std::endl;
  }

  /**
   *  Switch to HTM:
   *
   */
  void
  HTM::onSwitchTo()
  {
    std::cout << "Switch to HTM" << endl;
  }
}

namespace stm {
  /**
   *  HTM initialization
   */
  template<>
  void initTM<HTM>()
  {
      // set the name
      stms[HTM].name      = "HTM";

      // set the pointers
      stms[HTM].begin     = ::HTM::begin;
      stms[HTM].commit    = ::HTM::commit_rw;//o;
      stms[HTM].read      = ::HTM::read_rw;//o;      
      stms[HTM].read_promo= ::HTM::read_rw_promo;//o
      stms[HTM].write     = ::HTM::write_rw;//o;
      stms[HTM].rollback  = ::HTM::rollback;
      stms[HTM].irrevoc   = ::HTM::irrevoc;
      stms[HTM].switcher  = ::HTM::onSwitchTo;
      stms[HTM].privatization_safe = true;
  }
}
