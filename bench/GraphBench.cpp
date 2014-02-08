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

#include "Graph.hpp"
#include <time.h>



/**
 *  Step 3:
 *    Declare an instance of the data type, and provide init, test, and verify
 *    functions
 */

/*** the list we will manipulate in the experiment */
Graph* SET;
int elems [32];
int startelems = 0;

/*** Initialize the counter */
void bench_init()
{
  //    SET = new Graph();
  SET = (Graph*)hcmalloc(sizeof(Graph));
  new (SET) Graph();
  for(int i=0;i<32;i++){
    elems[i] = 0;
  }

// warm up the datastructure
    //
    // NB: if we switch to CGL, we can initialize without transactions
  /*  TM_BEGIN(atomic){//_FAST_INITIALIZATION();
    for (uint32_t w = 0; w < CFG.elements; w+=2){
            startelems++;
      //if(w%100==0)std::cout << "insert el" << w <<" "<< CFG.elements << std::endl;
      SET->insert(w TM_PARAM);
    }
  }
  TM_END;//_FAST_INITIALIZATION();*/
    std::cout << "start elems  " << startelems << std::endl;
}

/*** Run a bunch of increment transactions */
void bench_test(uintptr_t id, uint32_t* seed)
{
  // std::cout << "id " << id << " " <<(uint64_t)(*id) << std::endl;
  //TM_BEGIN(atomic){
    for(uint64_t o=0; o<CFG.ops; o++){
      uint64_t val = rand_r(seed) % CFG.elements;
      uint64_t act = rand_r(seed) % 100;
      bool res = false;
      if (act < CFG.lookpct) {
	TM_BEGIN(atomic) {
	  std::cout << "lookup" << std::endl;
	  //	val = 2000;
	  //SET->lookup(val TM_PARAM);
	//val = 1999;
	  SET->lookupVertex(val TM_PARAM);
	} TM_END;
      }
      else if (act < CFG.inspct) {
	//bool res = false;
	const uint64_t num_edges = 4;
	uint64_t edges[num_edges] = {1,4,7,9};
	TM_BEGIN(atomic) {
	  std::cout << "inser" << std::endl;
	  res = SET->insertVertex(val, edges, num_edges TM_PARAM);
	} TM_END;
	if(res){
	  //std::cout << "insert el " << val << std::endl; 
	  elems[id]++;
	}
      }
      else {
	//bool res =false;
	TM_BEGIN(atomic) {
	  std::cout << "rem "<<std::endl;
	  res = SET->removeVertex(val TM_PARAM);
	} TM_END;
	if(res){//std::cout << "remove el " << val << std::endl; 
	  elems[id]--;
	}
      }
    }
    //}
//TM_END;
}

/*** Ensure the final state of the benchmark satisfies all invariants */
bool bench_verify() { 
  int sum = 0;
  for(int i=0; i<16; i++){
    sum += elems[i];
    std::cout << "tid " << i << " : " << elems[i] << std::endl; 
  }
  std::cout << "sum " << sum << " sum + startelems " << sum+startelems << std::endl;
return SET->isSane(); }

/**
 *  Step 4:
 *    Include the code that has the main() function, and the code for creating
 *    threads and calling the three above-named functions.  Don't forget to
 *    provide an arg reparser.
 */

/*** Deal with special names that map to different M values */
void bench_reparse()
{
    if      (CFG.bmname == "")          CFG.bmname   = "Graph";
    else if (CFG.bmname == "Graph")      CFG.elements = 256;
}
