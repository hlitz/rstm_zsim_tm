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

#include "stm/btree.h"

/**
 *  Step 3:
 *    Declare an instance of the data type, and provide init, test, and verify
 *    functions
 */

/*** the list we will manipulate in the experiment */
BTREE SET;
int elems [32];
int startelems = 0;

int32_t comp(const void* a, const void* b){
  //std::cout << *((int64_t*)a) << " a b " << *((int64_t*)b) << std::endl;
  if(TM_READ_PROMOTED(*((int64_t*)a)) == TM_READ_PROMOTED(*((int64_t*)b)))
    return 0;
  else if(TM_READ_PROMOTED(*((int64_t*)a)) < TM_READ_PROMOTED(*((int64_t*)b)))
    return -1;
  else
    return 1;
}

uint64_t values[1024*1024];
/*** Initialize the counter */
void bench_init()
{
  for(uint32_t i =0; i<CFG.elements; i++){
    values[i] = i;
  }  
  SET = btree_Create(10, &comp );//(BTREE*)hcmalloc(sizeof(BTREE));
  //new (SET) BTREE();
  std::cout<<"warm up data structure" << std::endl;
  //    SET = new DList();
    // warm up the datastructure
  //bench_verify();
  TM_BEGIN(atomic) {//TM_BEGIN_FAST_INITIALIZATION();
    for (uint64_t i = 0; i < CFG.elements; i+=2){
      btree_Insert(SET, &values[i]);
    //std::cout<< "einsert " << res << std::endl;
  }
  }TM_END; //TM_END_FAST_INITIALIZATION();
  //bench_verify();
  for(int i=0;i<32;i++){
    elems[i] = 0;
  }

}

/*** Run a bunch of increment transactions */
void bench_test(uintptr_t id, uint32_t* seed)
{
    uint64_t val = rand_r(seed) % CFG.elements;
    uint32_t act = rand_r(seed) % 100;
    int res = 7;
    if (act < CFG.lookpct) {
        TM_BEGIN(atomic) {
	  void* ret = NULL;
	  res = btree_Search(SET, &values[val], ret);
        } TM_END;
    }
    else if (act < CFG.inspct) {
      //std::cout << "inserting " << val << std::endl; 
        TM_BEGIN(atomic) {
	  void* ret = NULL;
	  if(btree_Search(SET, &values[val], ret) !=0){
	    res = btree_Insert(SET, &values[val]);
	  }
        } TM_END;
	if(res==0){
	  //std::cout << "insert el " << val << " success " << res << std::endl; 
	  elems[id]++;
	}
    }
    else {
      //std::cout << "removing " << val << std::endl; 
        TM_BEGIN(atomic) {
	  res = btree_Delete(SET, &values[val]);
        } TM_END;
	if(res==0){
	  //  std::cout << "remove el " << val << " success " << res << std::endl; 
	  elems[id]--;
	}
    }
}

/*** Ensure the final state of the benchmark satisfies all invariants */
bool bench_verify() { 
  
  int sum = 0;
  for(int i=0; i<16; i++){
    sum += elems[i];
    std::cout << "tid " << i << " : " << elems[i] << std::endl; 
  }
  uint32_t sum_actual =0;
  std::cout << "sum " << sum << " sum + startelems " << sum+(CFG.elements/2) << std::endl;
    
    for(uint64_t i =0; i < CFG.elements; i++){ 
      TM_BEGIN(atomic) {
	void* ret = NULL;
	int res = 1;
	//std::cout << "serarch" << std::endl;
	res = btree_Search(SET, &values[i], ret);
	//std::cout << "verify" << &values[i] <<  " i n collection ? " << res << " ret val " << (uint64_t)ret <<" val " << values[i]<< std::endl;
	if(res==0){
	  sum_actual++;
	}
      } TM_END;
    }
    std::cout << "Sum actual " << sum_actual << std::endl;
    if(sum_actual != sum+(CFG.elements/2)){
    std::cout << "Verification FAILED !!!!!!! "<< std::endl;
  }
  return true;
}// btree_Print(); }

/**
 *  Step 4:
 *    Include the code that has the main() function, and the code for creating
 *    threads and calling the three above-named functions.  Don't forget to
 *    provide an arg reparser.
 */

/*** Deal with special names that map to different M values */
void bench_reparse()
{
    if (CFG.bmname == "") CFG.bmname = "DList";
}

