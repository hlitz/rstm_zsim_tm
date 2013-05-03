/**
 *  Copyright (C) 2011
 *  University of Rochester Department of Computer Science
 *    and
 *  Lehigh University Department of Computer Science and Engineering
 *
 * License: Modified BSD
 *          Please see the file LICENSE.RSTM for licensing information
 */

#include <stm/config.h>
#if defined(STM_CPU_SPARC)
#include <sys/types.h>
#endif

/**
 *  Step 1:
 *    Include the configuration code for the harness, and the API code.
 */
#include <iostream>
#include <api/api.hpp>
#include "bmconfig.hpp"

/**
 *  We provide the option to build the entire benchmark in a single
 *  source. The bmconfig.hpp include defines all of the important functions
 *  that are implemented in this file, and bmharness.cpp defines the
 *  execution infrastructure.
 */
#ifdef SINGLE_SOURCE_BUILD
#include "bmharness.cpp"
#endif

/**
 *  Step 2:
 *    Declare the data type that will be stress tested via this benchmark.
 *    Also provide any functions that will be needed to manipulate the data
 *    type.  Take care to avoid unnecessary indirection.
 */

//#include "Hash.hpp"
#define MAP_USE_RBTREE

#include "../stamp-0.9.10/lib/rbtree.h"

#include "../stamp-0.9.10/lib/map.h"

/**
 *  Step 3:
 *    Declare an instance of the data type, and provide init, test, and verify
 *    functions
 */

/*** the list we will manipulate in the experiment */
//HashTable* SET;
MAP_T* SET;

/*** Initialize the counter */
void bench_init()
{
  //SET = new HashTable();
  //SET = (MAP_T*)hcmalloc(sizeof(MAP_T));
  SET = MAP_ALLOC(NULL, NULL);
  //SET->init((CFG.elements/4));
  std::cout << "startup " << std::endl;
  // warm up the datastructure
    TM_BEGIN_FAST_INITIALIZATION();
    for (uint32_t w = 0; w < CFG.elements; w++){
      uint32_t seed = 7;
      int val = rand_r(&seed) % CFG.elements;
      //SET->insert(val TM_PARAM);
      MAP_INSERT(SET, val, val);
    }
    TM_END_FAST_INITIALIZATION();
}

/*** Run a bunch of increment transactions */
void bench_test(uintptr_t, uint32_t* seed)
{
  int val = rand_r(seed) % CFG.elements;
    uint32_t act = rand_r(seed) % 100;
    if (act < CFG.lookpct) {
      TM_BEGIN(/*atomic*/);
	  TMMAP_FIND(SET, val);
	  //            SET->lookup(val TM_PARAM);
	  TM_END();
    }
    else if (act < CFG.inspct) {
      TM_BEGIN(/*atomic*/);
	  TMMAP_INSERT(SET, val, val);
	  //            SET->insert(val TM_PARAM);
        TM_END();
    }
    else {
      TM_BEGIN(/*atomic*/);
	  //std::cout << " val " << val << std::endl;
	  TMMAP_REMOVE(SET, val);
	  //SET->remove(val TM_PARAM);
       TM_END();
    }
}

/*** Ensure the final state of the benchmark satisfies all invariants */
bool bench_verify() { /*return SET->isSane();*/ return true; }

/**
 *  Step 4:
 *    Include the code that has the main() function, and the code for creating
 *    threads and calling the three above-named functions.  Don't forget to
 *    provide an arg reparser.
 */

/*** Deal with special names that map to different M values */
void bench_reparse()
{
    if (CFG.bmname == "") CFG.bmname = "Map";
}
