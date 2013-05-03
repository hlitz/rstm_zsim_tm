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
 *  htm_el Implementation
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
__attribute__ ((noinline)) bool htm_el_trxaddwset(uint64_t addr, uint64_t data) {
  std::cout << "Calling dummy function hctrxaddwset, this text should not be shown, check pin instrumentation" << addr << std::endl; 
  return false;
}

__attribute__ ((noinline)) bool htm_el_trxaddrset(uint64_t addr) {
  std::cout << "Calling dummy function hctrxaddwset, this text should not be shown, check pin instrumentation" << addr << std::endl; 
  return false;
}

__attribute__ ((noinline)) void htm_el_trxstart() {
  std::cout << "Calling dummy function hctrxstart, this text should not be shown, check pin instrumentation" << std::endl; 
}

__attribute__ ((noinline)) bool htm_el_trxcommit() {
   std::cout << "Calling dummy function hctrxcommit, this text should not be shown, check pin instrumentation" << std::endl; 
  return false;
}

__attribute__ ((noinline)) bool htm_el_trxrocommit() {
  std::cout << "Calling dummy function hctrxrocommit, this text should not be shown, check pin instrumentation" << std::endl;
  return false;
}


/**
 *  Declare the functions that we're going to implement, so that we can avoid
 *  circular dependencies.
 */
namespace {
  struct htm_el
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

  /**
   *  htm_el begin:
   */
  bool
  htm_el::begin(TxThread* tx)
  {
    
    //  tx->allocator.onTxBegin(); //probably needed
    // get a start time
    //tx->start_time = timestamp.val;
    //std::cout << "APP TM_BEGIN"<< std::endl;
    htm_el_trxstart();
    return false;
  }

  /**
   *  htm_el commit (read-only):
   */
  void
  htm_el::commit_ro(TxThread* tx)
  {
    //CFENCE;
    //WBR;
    //std::cout << "APP lets do ro commit" << std::endl;
    if(!htm_el_trxrocommit()){
      //     OnFirstWrite(tx, read_rw, write_rw, commit_rw);
      //      std::cout << std::endl << std::endl << "----------------------- Aborting RO ----------------------------------------"<< tx->id << std::endl << std::endl;
      //std::cout << " APP abort in commit_ro" << std::endl;
      
   tx->tmabort(tx);
    }
    //tx->allocator.onTxCommit();
      // read-only, so just reset lists
      //tx->r_orecs.reset();
    //std::cout << " APP commit_ro " << std::endl;
   
      OnReadOnlyCommit(tx);
    //if(!hctrxcommit()) tx->abort(tx);
  }

  /**
   *  htm_el commit (writing context):
   *
   *    Get all locks, validate, do writeback.  Use the counter to avoid some
   *    validations.
   */
  void
  htm_el::commit_rw(TxThread* tx)
  {
    //  std::cout << "APP lets do rw commit" << std::endl;
    
    bool res = htm_el_trxcommit(); 
    // std::cout << "MIDDLE lets do rw commit" << std::endl;
    
    if(!res) { 
      //tx->allocator.onTxAbort(); 
      //CFENCE;
      //std::cout << " APP abort in commit_rw" << std::endl;
   
      tx->tmabort(tx);
    
      //std::cout << "APP ABORT" << std::endl;
    }

      // increment the global timestamp since we have writes
      //faiptr(&timestamp.val);
    
    //else{
    // std::cout << " APP commit_rw" << std::endl;
   
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
   *  htm_el read (read-only transaction)
   *
   *    We use "check twice" timestamps in htm_el
   */
  void*
  htm_el::read_ro(STM_READ_SIG(tx,addr,))
  {
    
    if(!htm_el_trxaddrset((uint64_t)addr)){
      //std::cout << " APP abort in read_ro" << std::endl;
      tx->tmabort(tx);}
    else
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
   *  htm_el read (writing transaction)
   */
  void*
  htm_el::read_rw(STM_READ_SIG(tx,addr,mask))
  {
    //    std::cout << "read_rw mask: " << cntr++ <<std::endl;
    
    if(!htm_el_trxaddrset((uint64_t)addr)){
      //std::cout << " APP abort in read_rw" << std::endl;
   
      tx->tmabort(tx);}
    else
    //std::cout << "read_ro mask: " << cntr++ << std::endl;
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
   *  htm_el write (read-only context)
   */
  void
  htm_el::write_ro(STM_WRITE_SIG(tx,addr,val,mask))
  {
    if((addr&0x7)!=0){printf("unaligend adddress")};
    printf("mask %i", mask);
    //hctrxaddwset((uint64_t)addr);
    //std::cout << std::hex << "write_ro addr: " << addr << " data " << val << std::endl;
    if(!htm_el_trxaddwset((uint64_t)addr, (uint64_t)val)){
      //std::cout << " APP abort in write_ro" << std::endl;
      
      tx->tmabort(tx);
    }
    
    //std::cout << "read_ro mask: " << cntr++ << std::endl;
      

    //*addr = val;
      // add to redo log
      //tx->writes.insert(WriteSetEntry(STM_WRITE_SET_ENTRY(addr, val, mask)));
      //OnFirstWrite(tx, read_rw, write_rw, commit_rw);
  }

  /**
   *  htm_el write (writing context)
   */
  void
  htm_el::write_rw(STM_WRITE_SIG(tx,addr,val,mask))
  {    

    //std::cout << std::hex << "write_rw addr: " << addr << " data " << val << std::endl;
    //hctrxaddwset((uint64_t)addr);
    //std::cout << "write_rw mask: " << *mask << std::endl;
    if(!htm_el_trxaddwset((uint64_t)addr, (uint64_t)val)){
      //std::cout << " APP abort in write_rw" << std::endl;
   
      tx->tmabort(tx);}
    //   *addr = val;
   // add to redo log
    //tx->writes.insert(WriteSetEntry(STM_WRITE_SET_ENTRY(addr, val, mask)));
  }

  /**
   *  htm_el unwinder:
   */
  stm::scope_t*
  htm_el::rollback(STM_ROLLBACK_SIG(tx, except, len))
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
   *  htm_el in-flight irrevocability:
   */
  bool
  htm_el::irrevoc(TxThread*)
  {
      return false;
  }

  /**
   *  htm_el validation
   */
  void
  htm_el::validate(TxThread* tx)
  {
    //std::cout << "validate!"<< std::endl;
      // validate
      /*foreach (OrecList, i, tx->r_orecs) {
          uintptr_t ivt = (*i)->v.all;
          // if unlocked and newer than start time, abort
          if ((ivt > tx->start_time) && (ivt != tx->my_lock.all))
              tx->tmabort(tx);
	      }*/
  }

  /**
   *  Switch to htm_el:
   *
   *    The timestamp must be >= the maximum value of any orec.  Some algs use
   *    timestamp as a zero-one mutex.  If they do, then they back up the
   *    timestamp first, in timestamp_max.
   */
  void
  htm_el::onSwitchTo()
  {
    timestamp.val = MAXIMUM(timestamp.val, timestamp_max.val);
  }
}

namespace stm {
  /**
   *  htm_el initialization
   */
  template<>
  void initTM<htm_el>()
  {
      // set the name
      stms[htm_el].name      = "htm_el";

      // set the pointers
      stms[htm_el].begin     = ::htm_el::begin;
      stms[htm_el].commit    = ::htm_el::commit_ro;
      stms[htm_el].read      = ::htm_el::read_ro;
      stms[htm_el].write     = ::htm_el::write_ro;
      stms[htm_el].rollback  = ::htm_el::rollback;
      stms[htm_el].irrevoc   = ::htm_el::irrevoc;
      stms[htm_el].switcher  = ::htm_el::onSwitchTo;
      stms[htm_el].privatization_safe = true;
  }
}
