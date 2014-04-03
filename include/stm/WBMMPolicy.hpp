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
 *  In order to get allocation and deallocation to work correctly inside of a
 *  speculative transactional region, we need to be sure that a doomed
 *  transaction cannot access memory that has been returned to the OS.
 *
 *  WBMMPolicy is RSTM's variant of epoch-based reclamation.  It is similar
 *  to proposals by [Fraser PhD 2003] and [Hudson ISMM 2006].
 *
 *  Note that this file has real code in it, and that code gets inlined into
 *  many places.  It's not pretty, and we may eventually want to reduce the
 *  footprint of this file on the rest of the project.
 */

#ifndef WBMMPOLICY_HPP__
#define WBMMPOLICY_HPP__

#include <stdlib.h>
#include <stm/config.h>
#include "stm/MiniVector.hpp"
#include "stm/metadata.hpp"
#include <iostream>
#include <vector>
#include "stm/lib_hicamp.h"

namespace stm
{
  /*** forward declare the threadcount used by TxThread */
  extern pad_word_t threadcount;

  /*** store every thread's counter */
  extern pad_word_t trans_nums[MAX_THREADS];

  /*** Node type for a list of timestamped void*s */
  struct limbo_t
  {
      /*** Number of void*s held in a limbo_t */
      static const uint32_t POOL_SIZE = 32;

      /*** Set of void*s */
      void*     pool[POOL_SIZE];

      /*** Timestamp when last void* was added */
      uint32_t  ts[MAX_THREADS];

      /*** # valid timestamps in ts, or # elements in pool */
      uint32_t  length;

      /*** NehelperMin pointer for the limbo list */
      limbo_t*  older;

      /*** The constructor for the limbo_t just zeroes out everything */
      limbo_t() : length(0), older(NULL) { }
  };

  /**
   * WBMMPolicy
   *  - log allocs and frees from within a transaction
   *  - on abort, free any allocs
   *  - on commit, replay any frees
   *  - use epochs to prevent reclamation during a doomed transaction's
   *    execution
   */
  class WBMMPolicy
  {
      /*** location of my timestamp value */
      volatile uintptr_t* my_ts;

      /*** As we mark things for deletion, we accumulate them here */
      limbo_t* prelimbo;

      /*** sorted list of timestamped reclaimables */
      limbo_t* limbo;

      /*** List of objects to delete if the current transaction commits */
      //AddressList frees;
    std::vector<uint64_t> frees;
      /*** List of objects to delete if the current transaction aborts */
      //AddressList allocs;
    std::vector<uint64_t> allocs;
    uint32_t tid;
      /**
       *  Schedule a pointer for reclamation.  Reclamation will not happen
       *  until enough time has passed.
       */
      void schedForReclaim(void* ptr)
      {
          // insert /ptr/ into the prelimbo pool and increment the pool size
          prelimbo->pool[prelimbo->length++] = ptr;
          // if prelimbo is not full, we're done
          if (prelimbo->length != prelimbo->POOL_SIZE)
              return;
          // if prelimbo is full, we have a lot more work to do
          handle_full_prelimbo();
      }

      /**
       *  This code is the cornerstone of the WBMMPolicy.  We buffer lots of
       *  frees onto a prelimbo list, and then, at some point, we must give
       *  that list a timestamp and tuck it away until the timestamp expires.
       *  This is how we do it.
       */
      NOINLINE void handle_full_prelimbo();

    public:

      /**
       *  Constructing the DeferredReclamationMMPolicy is very easy
       *  Null out the timestamp for a particular thread.  We only call this
       *  at initialization.
       */
      WBMMPolicy()
	: prelimbo(new limbo_t()), limbo(NULL)//, frees(128), allocs(128)
      { }

      /**
       *  Since a TxThread constructs its allocator before it gets its id, we
       *  need the TxThread to inform the allocator of its id from within the
       *  constructor, via this method.
       */
    void setID(uint32_t id) { tid = id; my_ts = &trans_nums[id].val; }
    

      /*** Wrapper to thread-specific allocator for allocating memory */
      void* txAlloc(size_t const &size)
      {

          void* ptr = hcmalloc(size);
          /*if ((*my_ts)&1)
	    allocs.push_back((uint64_t)ptr);//insert(ptr);
	  */
	  //AddressList::iterator i, e;
	  //for (i = allocs.begin(), e = allocs.end(); i != e; ++i)
	  //if(tid==1)  std::cout << "txalloc: " << *(allocs.end()) << " " << "begin memaddr: " <<&(*(allocs.begin())) << " tid: " << tid << " size: " << allocs.size() << std::endl;

          return ptr;
      }

      /*** Wrapper to thread-specific allocator for freeing memory */
      void txFree(void* ptr)
      {
	if ((*my_ts)&1){
	  //std::cout << "shed for reclaim " << ptr << std::endl;
	  //frees.push_back((uint64_t)ptr);//insert(ptr);
	}
	else{
	    //std::cout << "WB txFree adr " << ptr<< std::endl;
	    hcfree(ptr);
	  }
      }

      /*** On begin, move to an odd epoch and start logging */
      void onTxBegin() { *my_ts = 1 + *my_ts; }

      /*** On abort, unroll allocs, clear lists, exit epoch */
      void onTxAbort()
      {
	
	//if(tid==1) std::cout << "ontxabort: "<<allocs.size()<< " tid: " << tid << " beginptr: " << &(*(allocs.begin())) << std::endl;
	
        if(allocs.size()>0){

	  //Ad i, e;
          for (std::vector<uint64_t>::iterator i = allocs.begin() /*,e = allocs.end()*/; i <allocs.end()/*!=e*/; i++){
	    //std::cout << "WBonTxAbort freein adr " << *i << std::endl;
	    hcfree((void*)*i);
	  }
	}
	frees.clear();//reset();
	//allocs.reset();
	allocs.clear();
	*my_ts = 1+*my_ts;
      }

      /*** On commit, perform frees, clear lists, exit epoch */
      void onTxCommit()
      {
	//AddressList::iterator i, e;
	for (std::vector<uint64_t>::iterator i = frees.begin()/*, e = frees.end()*/; i<frees.end() /*!= e*/; i++)
	  schedForReclaim((void*)*i);
	
	//if(tid==1) std::cout << "ontxcommit: "<<allocs.size()<< " tid: " << tid << std::endl;

	frees.clear();//reset();
	//allocs.reset();
	allocs.clear();
	*my_ts = 1+*my_ts;
      }
  }; // class stm::WBMMPolicy

} // namespace stm

#endif // WBMMPOLICY_HPP__
