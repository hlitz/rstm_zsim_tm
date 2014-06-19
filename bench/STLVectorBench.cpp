/**
 *  Copyright (C) 2011
 *  University of Rochester Department of Computer Science
 *    and
 *  Lehigh University Department of Computer Science and Engineering
 *
 * License: Modified BSD
 *          Please see the file LICENSE.RSTM/ for licensing information
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
#include <vector>
#include <stm/mvcc_allocator.h>
//using namespace std;

std::vector <uint64_t, mvcc_allocator <uint64_t> > vec;

/**
 *  Step 3:
 *    Declare an instance of the data type, and provide init, test, and verify
 *    functions
 */

/*** Initialize an array that we can use for our MCAS test */
void bench_init()
{
  //vec.resize(1000);
  //matrix = (int*)hcmalloc(CFG.elements * sizeof(int));
  // matrix[0] = 0;
}

int elems[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
/*** Run a bunch of random transactions */
void bench_test(uintptr_t id, uint32_t* seed)
{
    // cache the seed locally so we can restore it on abort
    //
    // NB: volatile needed because using a non-volatile local in conjunction
    //     with a setjmp-longjmp control transfer is undefined, and gcc won't
    //     allow it with -Wall -Werror.
    volatile uint32_t local_seed = *seed;

    uint64_t val = rand() % CFG.elements;
    uint64_t act = rand() % 100;
    
    //printf("insp %i\n", CFG.inspct);
    //cout << "act " << act << endl;
      bool found = false;
      if (act < CFG.lookpct) {
	TM_BEGIN(atomic) {
	  for(auto it = vec.begin(); it != vec.end(); it++){
	    if(val == *it){
	      found = true;
	      break;
	    } 
	  }
	} TM_END;
      }
      else if (act < CFG.inspct) {
	//std::cout << "inserting " << vec.size() << std::endl;
	TM_BEGIN(atomic) {
	  //for(auto it = vec.begin(); it != vec.end(); it++){
	    //if(*it == val);
	    vec.push_back(val);
	    //}
	    elems[id]++;
	} TM_END;
	//cout << "insert done" << endl;
	//std::cout << "vec size " << vec.size() << std::endl;
      }
      else{
	bool erased = false;
	//std::cout << "Erasing " << std::endl;
	TM_BEGIN(atomic) {
	  for(auto it = vec.begin(); it != vec.end(); it++){
	    if(*it == val);
	    vec.erase(it);
	    erased = true;
	    break;
	  }
	} TM_END;
	if(erased) elems[id]--;
	//std::cout << "erase done" << std::endl;
      }
      *seed = local_seed;
}

/*** Ensure the final state of the benchmark satisfies all invariants */
bool bench_verify() { 
  int sum =0;
  for(int i =0; i< 32; i++){
    sum += elems[i];
  }
  std::cout << "sum : " << sum << " size " << vec.size() << std::endl;
  return sum == vec.size();}

/**
 *  Step 4:
 *    Include the code that has the main() function, and the code for creating
 *    threads and calling the three above-named functions.  Don't forget to
 *    provide an arg reparser.
 */

/*** Deal with special names that map to different M values */
void bench_reparse()
{
    if      (CFG.bmname == "")          CFG.bmname   = "MCAS";
}
