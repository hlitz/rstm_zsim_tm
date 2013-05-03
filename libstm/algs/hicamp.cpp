/**
 *  Copyright (C) 2011
 *  University of Rochester Department of Computer Science
 *    and
 *  Lehigh University Department of Computer Science and Engineering
 *
 * License: Modified BSD
 *          Please see the file LICENSE.RSTM for licensing information
 */

/**
 *  HICAMP Implementation
 *
 *    This STM very closely resembles the GV1 variant of TL2.  That is, it uses
 *    orecs and lazy acquire.  Its clock requires everyone to increment it to
 *    commit writes, but this allows for read-set validation to be skipped at
 *    commit time.  Most importantly, there is no in-flight validation: if a
 *    timestamp is greater than when the transaction sampled the clock at begin
 *    time, the transaction aborts.
 */

#include "../profiling.hpp"
#include "algs.hpp"
#include "RedoRAWUtils.hpp"
#include <iostream>
#include <vector>

using stm::TxThread;
using stm::timestamp;
using stm::timestamp_max;
using stm::WriteSet;
using stm::OrecList;
using stm::UNRECOVERABLE;
using stm::WriteSetEntry;
using stm::orec_t;
using stm::get_orec;

/* dummy function to be instrumented by zsim*/
__attribute__ ((noinline)) void hctrxaddwset(uint64_t addr) {
      std::cout << "Calling dummy function hctrxaddwset, this text should not be shown, check pin instrumentation" << addr << std::endl; 
}

__attribute__ ((noinline)) uint64_t hctrxstart() {
  std::cout << "Calling dummy function hctrxstart, this text should not be shown, check pin instrumentation" << std::endl; 
  return 0;
}

__attribute__ ((noinline)) bool hctrxcommit() {
   std::cout << "Calling dummy function hctrxcommit, this text should not be shown, check pin instrumentation" << std::endl; 
  return false;
}

__attribute__ ((noinline)) bool hctrxrocommit() {
  std::cout << "Calling dummy function hctrxrocommit, this text should not be shown, check pin instrumentation" << std::endl;
  return false;
}
__attribute__ ((noinline)) bool graph_detection(int h) {
  std::cout << "Calling dummy function graph_detection, this text should not be shown, check pin instrumentation" << std::endl;
  return false;
}

__attribute__ ((noinline)) bool hicamp_rw_insert(const void *addr,  bool rw) {
  std::cout << "Calling dummy function hicamp_rw_insert, this text should not be shown, check pin instrumentation" << std::endl;
  return false;
}
__attribute__ ((noinline)) void insert_breakpoint(){
  std::cout << "now breaking " << std::endl;
}

using namespace std;
//vector< vector <uint64_t> >rd_sets(64);
//vector< vector <uint64_t> >wr_sets(64);

/**
 *  Declare the functions that we're going to implement, so that we can avoid
 *  circular dependencies.
 */
namespace {
  struct HICAMP
  {
      static TM_FASTCALL bool begin(TxThread*);
      static TM_FASTCALL void* read_ro(STM_READ_SIG(,,));
      static TM_FASTCALL void* read_rw(STM_READ_SIG(,,));
      static TM_FASTCALL void write_ro(STM_WRITE_SIG(,,,));
      static TM_FASTCALL void write_rw(STM_WRITE_SIG(,,,));
      static TM_FASTCALL void commit_ro(TxThread*);
      static TM_FASTCALL void commit_rw(TxThread*);

      static stm::scope_t* rollback(STM_ROLLBACK_SIG(,,));
      static bool irrevoc(TxThread*);
      static void onSwitchTo();
      static NOINLINE void validate(TxThread*);
  };
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
   *  HICAMP begin:
   */
  bool
  HICAMP::begin(TxThread* tx)
  {

    uint64_t begintime, endtime;
    tx->allocator.onTxBegin(); //probably needed
    // get a start time
    //tx->start_time = timestamp.val;
    //std::cout << "APP TM_BEGIN"<< std::endl;
    uint64_t wait = hctrxstart();
    if(wait>1){
      //printf("begin wait\n");
      begintime = rdtsc();
      endtime = begintime;
      while(begintime+wait>endtime && begintime+wait>begintime){
	endtime = rdtsc();
	//cntr[tx->id]++;
	//printf("wait %lx begin %lx end-b %lx ", wait, begintime, endtime-begintime);
      }
    } 
    return false;
  }

  /**
   *  HICAMP commit (read-only):
   */
  void
  HICAMP::commit_ro(TxThread* tx)
  {
    
    //    rd_sets[tx->id].push_back(0x0UL);
    //wr_sets[tx->id].push_back(0x0UL);
    
    //std::cout << "APP lets do ro commit" << std::endl;
    if(!hctrxrocommit()){
      //     OnFirstWrite(tx, read_rw, write_rw, commit_rw);
      //      std::cout << std::endl << std::endl << "----------------------- Aborting RO ----------------------------------------"<< tx->id << std::endl << std::endl;
      tx->allocator.onTxAbort(); 
      tx->tmabort(tx);
    }
    tx->allocator.onTxCommit();
      // read-only, so just reset lists
      //tx->r_orecs.reset();
      OnReadOnlyCommit(tx);
    //if(!hctrxcommit()) tx->abort(tx);
  }

  /**
   *  HICAMP commit (writing context):
   *
   *    Get all locks, validate, do writeback.  Use the counter to avoid some
   *    validations.
   */
  void
  HICAMP::commit_rw(TxThread* tx)
  {
    /*rd_sets[tx->id].push_back(0x0UL);
    wr_sets[tx->id].push_back(0x0UL);
    //    cout << tx->id << endl;
    //  std::cout << "APP lets do rw commit" << std::endl;
    if(tx->id==1){
      cnt++;
      //cout << cnt << endl;
      if(cnt == 40){
	for(int i =0; i< rd_sets[1].size(); i++){
	  cout << "T1 i "<< i << " RD " << rd_sets[1][i] << endl; 
	}
	for(int i =0; i< wr_sets[1].size(); i++){
	  cout << "T1 i "<< i << " WR " << wr_sets[1][i] << endl; 
	}
	for(int i =0; i< rd_sets[2].size(); i++){
	  cout << "T2 i "<< i << " RD " << rd_sets[2][i] << endl; 
	}
	for(int i =0; i< wr_sets[2].size(); i++){
	  cout << "T2 i "<< i << " WR " << wr_sets[2][i] << endl; 
	}
      }
      }*/
    bool res = hctrxcommit(); 
    
    if(!res) { 
      tx->allocator.onTxAbort(); 
      //CFENCE;

    
      tx->tmabort(tx);
    
      //std::cout << "APP ABORT" << std::endl;
    }

      // increment the global timestamp since we have writes
      //faiptr(&timestamp.val);
    
    //else{
    tx->allocator.onTxCommit(); 
    OnReadWriteCommit(tx, read_ro, write_ro, commit_ro);
      //std::cout << "END APP lets do rw commit" << std::endl;
      
      //tx->allocator.onTxCommit();
      //std::cout << "APP commit" << std::endl;
      //}
      // acquire locks
      /*foreach (WriteSet, i, tx->writes) {
          // get orec, read its version#
          orec_t* o = get_orec(i->addr);
          uintptr_t ivt = o->v.all;

          // lock all orecs, unless already locked
          if (ivt <= tx->start_time) {
              // abort if cannot acquire
              if (!bcasptr(&o->v.all, ivt, tx->my_lock.all))
                  tx->tmabort(tx);
              // save old version to o->p, remember that we hold the lock
              o->p = ivt;
              tx->locks.insert(o);
          }
          // else if we don't hold the lock abort
          else if (ivt != tx->my_lock.all) {
              tx->tmabort(tx);
          }
      }

      // increment the global timestamp since we have writes
      uintptr_t end_time = 1 + faiptr(&timestamp.val);

      // skip validation if nobody else committed
      if (end_time != (tx->start_time + 1))
          validate(tx);

      // run the redo log
      *///tx->writes.writeback();
      /*
      // release locks
      CFENCE;
      foreach (OrecList, i, tx->locks)
          (*i)->v.all = end_time;

      // clean-up
      tx->r_orecs.reset();
      *///tx->writes.reset();/*
      //tx->locks.reset();*/
  }

  /**
   *  HICAMP read (read-only transaction)
   *
   *    We use "check twice" timestamps in HICAMP
   */
  void*
  HICAMP::read_ro(STM_READ_SIG(tx,addr,))
  {
    //rd_sets[tx->id].push_back((uint64_t)addr);
    if(hicamp_rw_insert(addr, 0)){
      cout << "APP writeskew" << endl;
      insert_breakpoint(); //abort on write skew
    }
    //std::cout << "read_ro mask: " << cntr++ << std::endl;
    return *addr;
      // get the orec addr
      /*orec_t* o = get_orec(addr);

      // read orec, then val, then orec
      uintptr_t ivt = o->v.all;
      CFENCE;
      void* tmp = *addr;
      CFENCE;
      uintptr_t ivt2 = o->v.all;
      // if orec never changed, and isn't too new, the read is valid
      if ((ivt <= tx->start_time) && (ivt == ivt2)) {
          // log orec, return the value
          tx->r_orecs.insert(o);
          return tmp;
      }
      // unreachable
      tx->tmabort(tx);
      return NULL;*/
  }

  /**
   *  HICAMP read (writing transaction)
   */
  void*
  HICAMP::read_rw(STM_READ_SIG(tx,addr,mask))
  {
    if(hicamp_rw_insert(addr, 0)){
      cout << "APP writeskew" << endl;
      insert_breakpoint(); //abort on write skew
    }
    //rd_sets[tx->id].push_back((uint64_t)addr);
    //    std::cout << "read_rw mask: " << cntr++ <<std::endl;
    return *addr;
    /*
      // check the log for a RAW hazard, we expect to miss
      WriteSetEntry log(STM_WRITE_SET_ENTRY(addr, NULL, mask));
      bool found = tx->writes.find(log);
      REDO_RAW_CHECK(found, log, mask);

      // get the orec addr
      orec_t* o = get_orec(addr);

      // read orec, then val, then orec
      uintptr_t ivt = o->v.all;
      CFENCE;
      void* tmp = *addr;
      CFENCE;
      uintptr_t ivt2 = o->v.all;

      // fixup is here to minimize the postvalidation orec read latency
      REDO_RAW_CLEANUP(tmp, found, log, mask);
      // if orec never changed, and isn't too new, the read is valid
      if ((ivt <= tx->start_time) && (ivt == ivt2)) {
          // log orec, return the value
          tx->r_orecs.insert(o);
          return tmp;
      }
      tx->tmabort(tx);
      // unreachable
      return NULL;*/
  }

  /**
   *  HICAMP write (read-only context)
   */
  void
  HICAMP::write_ro(STM_WRITE_SIG(tx,addr,val,mask))
  {
    /*
    if(hicamp_rw_insert(addr, 1)){
      cout << "APP writeskew" << endl;
      insert_breakpoint(); //abort on write skew
    }*/
    //    wr_sets[tx->id].push_back((uint64_t)addr);
    hctrxaddwset((uint64_t)addr);
    //std::cout << "write_ro mask: " << *mask << std::endl;
    *addr = val;
      // add to redo log
      //tx->writes.insert(WriteSetEntry(STM_WRITE_SET_ENTRY(addr, val, mask)));
      OnFirstWrite(tx, read_rw, write_rw, commit_rw);
  }

  /**
   *  HICAMP write (writing context)
   */
  void
  HICAMP::write_rw(STM_WRITE_SIG(tx,addr,val,mask))
  {   
    /*
    if(hicamp_rw_insert(addr, 1)){
      cout << "APP writeskew" << endl;
      insert_breakpoint(); //abort on write skew
    }
    */
    //wr_sets[tx->id].push_back((uint64_t)addr);
 
    hctrxaddwset((uint64_t)addr);
    //std::cout << "write_rw mask: " << *mask << std::endl;
    *addr = val;
   // add to redo log
    //tx->writes.insert(WriteSetEntry(STM_WRITE_SET_ENTRY(addr, val, mask)));
  }

  /**
   *  HICAMP unwinder:
   */
  stm::scope_t*
  HICAMP::rollback(STM_ROLLBACK_SIG(tx, except, len))
  {
      PreRollback(tx); 

      // Perform writes to the exception object if there were any... taking the
      // branch overhead without concern because we're not worried about
      // rollback overheads.
      //STM_ROLLBACK(tx->writes, except, len);

      // release the locks and restore version numbers
      //foreach (OrecList, i, tx->locks)
      //    (*i)->v.all = (*i)->p;

      // undo memory operations, reset lists
      //tx->r_orecs.reset();
      //tx->writes.reset();
      // tx->locks.reset();
      return PostRollback(tx, read_ro, write_ro, commit_ro);
  }

  /**
   *  HICAMP in-flight irrevocability:
   */
  bool
  HICAMP::irrevoc(TxThread*)
  {
      return false;
  }

  /**
   *  HICAMP validation
   */
  void
  HICAMP::validate(TxThread* tx)
  {
    std::cout << "validate!"<< std::endl;
      // validate
      /*foreach (OrecList, i, tx->r_orecs) {
          uintptr_t ivt = (*i)->v.all;
          // if unlocked and newer than start time, abort
          if ((ivt > tx->start_time) && (ivt != tx->my_lock.all))
              tx->tmabort(tx);
	      }*/
  }

  /**
   *  Switch to HICAMP:
   *
   *    The timestamp must be >= the maximum value of any orec.  Some algs use
   *    timestamp as a zero-one mutex.  If they do, then they back up the
   *    timestamp first, in timestamp_max.
   */
  void
  HICAMP::onSwitchTo()
  {
    timestamp.val = MAXIMUM(timestamp.val, timestamp_max.val);
  }
}

namespace stm {
  /**
   *  HICAMP initialization
   */
  template<>
  void initTM<HICAMP>()
  {
      // set the name
      stms[HICAMP].name      = "HICAMP";

      // set the pointers
      stms[HICAMP].begin     = ::HICAMP::begin;
      stms[HICAMP].commit    = ::HICAMP::commit_ro;
      stms[HICAMP].read      = ::HICAMP::read_ro;
      stms[HICAMP].write     = ::HICAMP::write_ro;
      stms[HICAMP].rollback  = ::HICAMP::rollback;
      stms[HICAMP].irrevoc   = ::HICAMP::irrevoc;
      stms[HICAMP].switcher  = ::HICAMP::onSwitchTo;
      stms[HICAMP].privatization_safe = true;
  }
}
