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

//#include "DList.hpp"

/**
 *  Step 3:
 *    Declare an instance of the data type, and provide init, test, and verify
 *    functions
 */

/*** the list we will manipulate in the experiment */
//DList* SET;
uint32_t *array;
/*** Initialize the counter */
void bench_init()
{
  array = (uint32_t*)hcmalloc(sizeof(int)*CFG.elements);
  //SET->DListInit();
    // warm up the datastructure
  /*, 2, 6, 0TM_BEGIN_FAST_INITIALIZATION();
    for (uint32_t i = 0; i < CFG.elements; i+=2){
      int u = i*1000;
      //      std::cout << "START " <<u << std::endl; 
        SET->insert(u TM_PARAM);
    }
    TM_END_FAST_INITIALIZATION();*/
}

const uint32_t ACCESS = 10;

/*** Run a bunch of increment transactions */
void bench_test(uintptr_t, uint32_t* seed)
{
    uint32_t act = rand_r(seed) % 100;
  
     if (act < CFG.lookpct) {
       TM_BEGIN(atomic){
	 uint32_t rand = rand_r(seed) % CFG.elements;
	 for(uint32_t e=0; e<1; e++){
	   rand = rand_r(&rand) % CFG.elements;
	   
	 }
	 for(uint32_t i=0;i<ACCESS;i++){
	   uint32_t offset = rand_r(seed) % CFG.elements;
	   if(TM_READ(*(array+offset))==0xBABE){
	     std::cout << "wrote babe" <<std::endl;
	   }
	 }
	 uint32_t offset = rand_r(seed) % CFG.elements;
	 TM_WRITE(*(array+offset+10), rand);
	 
       }TM_END;
     }
     else{
       TM_BEGIN(atomic){
	 uint32_t rand = rand_r(seed) % CFG.elements;
	 for(uint32_t e=0; e<1; e++){
	   rand = rand_r(&rand) % CFG.elements;
	   
	 }
	
	 for(uint32_t i=0;i<ACCESS;i++){
	   rand = rand_r(&rand) % 60000;//CFG.elements;
	   uint32_t offset = rand_r(seed) % CFG.elements;
	   //std::cout << "rand " << rand << std::endl;
	   TM_WRITE(*(array+offset), rand);
	 }	 
       }TM_END;
     
     }
}
  /*TM_BEGIN(atomic){
    bool found;
    uint32_t act = rand_r(seed) % 100;
    uint32_t rand = rand_r(seed) % CFG.elements;
    uint32_t rand1 = rand_r(seed) % CFG.elements;
    uint32_t rand2 = rand_r(seed) % CFG.elements;
    for(uint32_t i =0; i<rand; i++){
      if(TM_READ(*(array+i))==0xBABE){
	      //if(*(array+i))==0xBABE){
	found = true;
	//	break;
	}
	}
    if(found)
      TM_WRITE(*(array+rand1), rand1);
    //*(array+rand1) = rand1;
    else
      TM_WRITE(*(array+rand), rand2);
    //*(array+rand) =  rand2;
  }TM_END;
  TM_BEGIN(atomic){
    uint32_t rand3 = rand_r(seed) % CFG.elements;
    TM_WRITE(*(array+rand3), rand3);
     
    }TM_END;
}*/
	  
    /*
    if (act < CFG.lookpct) {
        TM_BEGIN(atomic) {
	  //val = rand_r(seed) % CFG.elements;
	  uint32_t val = rand_r(seed) % CFG.elements;
   
	  //val = rand_r(seed) % CFG.elements;
          //SET->remove(val TM_PARAM);
	  //val = rand_r(seed) % CFG.elements;
          //SET->insert(val TM_PARAM);
	  //std::cout << "read" << std::endl;
	  //for(int i=0; i<100; i++){
	  //val = i;//rand_r(seed) % CFG.elements;
	    //	      std::cout << "READ " << CFG.elements << " val " << val << std::endl;
	    //SET->lookup(val TM_PARAM);
	  //}
	  val = (rand_r(seed) % (CFG.elements*1000))+1000;
	  //std::cout << "---WRITE " << CFG.elements << " val " << val << std::endl;
	  //std::cout << val << std::endl;
          //SET->insert(val TM_PARAM);

        } TM_END;
	}
    else if (act < CFG.inspct) {
        TM_BEGIN(atomic) { 
	  //	  std::cout << "write" << std::endl;
            SET->insert(val TM_PARAM);
        } TM_END;
	}
    else {
        TM_BEGIN(atomic) {
	  //std::cout << "remove" << std::endl;
           uint32_t val = rand_r(seed) % CFG.elements;  
	   SET->remove(val TM_PARAM);
	   SET->insert(val TM_PARAM);
	   std::cout << val << std::endl;
        } TM_END;
	}
   
}*/

/*** Ensure the final state of the benchmark satisfies all invariants */
bool bench_verify() { return true;}//return SET->isSane(); }

/**
 *  Step 4:
 *    Include the code that has the main() function, and the code for creating
 *    threads and calling the three above-named functions.  Don't forget to
 *    provide an arg reparser.
 */

/*** Deal with special names that map to different M values */
void bench_reparse()
{
    if (CFG.bmname == "") CFG.bmname = "Vector";
}

