/**
 *  Copyright (C) 2011
 *  University of Rochester Department of Computer Science
 *    and
 *  Lehigh University Department of Computer Science and Engineering
 *
 * License: Modified BSD
 *          Please see the file LICENSE.RSTM for licensing information
 */
/*
#include <stm/config.h>
#if defined(STM_CPU_SPARC)
#include <sys/types.h>
#endif
*/
/**
 *  Step 1:
 *    Include the configuration code for the harness, and the API code.
 */
#include <iostream>
#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

//#include <api/api.hpp>
//#include "bmconfig.hpp"

/**
 *  We provide the option to build the entire benchmark in a single
 *  source. The bmconfig.hpp include defines all of the important functions
 *  that are implemented in this file, and bmharness.cpp defines the
 *  execution infrastructure.
 */
/*#ifdef SINGLE_SOURCE_BUILD
#include "bmharness.cpp"
#endif
*/
/**
 *  Step 2:
 *    Declare the data type that will be stress tested via this benchmark.
 *    Also provide any functions that will be needed to manipulate the data
 *    type.  Take care to avoid unnecessary indirection.
 */

//#include "Hash.hpp"
//#define MAP_USE_RBTREE

//#include "../stamp-0.9.10/lib/rbtree.h"
#include "thread.h"
#include "tm.h"
#include "list.h"
#include "thread.h"
#include <time.h>
//THREAD_MUTEX_T lock;
using namespace std;
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

typedef struct tls{
  int64_t inserts;
  uint64_t pad[7];
}tls_t;

tls_t* tls_array;

/**
 *  Step 3:
 *    Declare an instance of the data type, and provide init, test, and verify
 *    functions
 */

/*** the list we will manipulate in the experiment */
//HashTable* SET;
list_t* LIST;

typedef struct config{
  uint32_t elements;
  uint32_t lookpct;
  char* bmname;
  uint32_t inspct;
  int runs;
}CFG_t;



/*** Initialize the counter */
void bench_init(void* _CFG)
{
  CFG_t* CFG = (CFG_t*)_CFG;
  //SET = new HashTable();
  //SET = (MAP_T*)hcmalloc(sizeof(MAP_T));
  LIST = PLIST_ALLOC(NULL);
  //SET->init((CFG.elements/4));
  std::cout << "startup " << std::endl;
  // warm up the datastructure
  //TM_BEGIN(); //_FAST_INITIALIZATION();
  for (uint32_t w = 0; w < ((CFG_t*)CFG)->elements; w++){
      
      int val = rdtsc() % CFG->elements;
      //SET->insert(val TM_PARAM);
      PLIST_INSERT(LIST, (void*)val);
    }
    //TM_END();//_FAST_INITIALIZATION();
}



/*** Run a bunch of increment transactions */
void bench_test(void* _CFG)
{
  TM_THREAD_ENTER();
  CFG_t* CFG = (CFG_t*)_CFG;
    long tid = thread_getId();
  
  uint64_t val;
  uint64_t act;
  bool result = false;
  for(int i =0; i < CFG->runs; i++){
    act = rdtsc() % 100UL;
    val = rdtsc() % CFG->elements;
    if (act < CFG->lookpct) {
      TM_BEGIN();
      //printf("look act %i\n", act);
      TMLIST_FIND(LIST, (void*)val);
      //            SET->lookup(val TM_PARAM);
      TM_END();
    }
    else if (act < CFG->inspct) {
      //THREAD_MUTEX_LOCK(lock);        
      TM_BEGIN();
      //printf("ins act %i\n", act);
      result = TMLIST_INSERT(LIST, (void*)val);
      TM_END();
      //      THREAD_MUTEX_UNLOCK(lock);        
      if(result){
	tls_array[tid].inserts++;
      }
    }
    else {
      //THREAD_MUTEX_LOCK(lock);        
      TM_BEGIN(/*atomic*/);
      //       printf("rem act %i\n", act);
     //std::cout << " val " << val << std::endl;
      result = TMLIST_REMOVE(LIST, (void*)val);
      
      TM_END();
      //THREAD_MUTEX_UNLOCK(lock);        

      if(result){
	tls_array[tid].inserts--;
      }
    }
  }
  TM_THREAD_EXIT();

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
/*void bench_reparse()
{
    if (CFG.bmname == "") CFG.bmname = "Map";
    }*/


MAIN(argc, argv)
{
    /*
     * Initialization
     */
  //    THREAD_MUTEX_INIT(lock);
        
  if(argc!=6){
    printf("Usage: listbenchSTM64 <1> <2> <3> <4> <5> where,\n<1>: Number of elements\n<2>: Lookup percentage\n<3>Insertion percentage\n<4>Iterations\n<5>Number of threads\n");
    abort();
  }
    CFG_t cfg ;
    printf("args %i\n", argc);
    cfg.elements = atol(argv[1]);//16;
    cfg.lookpct = atol(argv[2]);
    //cfg.bmname = "map";
    cfg.inspct = atol(argv[3]);
    cfg.runs = atol(argv[4]);
    
    long numThread = atol(argv[5]);
    tls_array = (tls_t*)malloc(sizeof(tls_t)*numThread);
    SIM_GET_NUM_CPU(numThread);
    TM_STARTUP(numThread);
    P_MEMORY_STARTUP(numThread);
    thread_startup(numThread);
    bench_init((void*)&cfg);
    int32_t numstart=0;
    /*for(uint64_t i=0; i<cfg.elements; i++){
      if(MAP_CONTAINS(SET, i))
	numstart++;
	}*/
    printf("Elements in map at start: %i \n", numstart=PLIST_GETSIZE(LIST));
  struct timespec start, finish;
  double elapsed;

  clock_gettime(CLOCK_MONOTONIC, &start);
  
#ifdef OTM
#pragma omp parallel
    {
        bench_test((void*)&cfg);
    }

#else
    thread_start(bench_test, (void*)&cfg);
#endif
  clock_gettime(CLOCK_MONOTONIC, &finish);

  elapsed = (finish.tv_sec - start.tv_sec);
  elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
  std::cout << "time: " << (elapsed*1000) <<  std::endl;
    TM_SHUTDOWN();

    printf("mem shotydown \n");
    P_MEMORY_SHUTDOWN();

    thread_shutdown();
    int64_t numelem = 0;
    int64_t numthread = 0;
    for(int64_t i=0; i<numThread; i++){
      numthread += tls_array[i].inserts;
      cout <<"thread i: " << i <<    " inserts: " <<  tls_array[i].inserts << endl;
      //      printf("thread i: %i inserts: %l\n", i, tls_array[i].inserts);
    }
   /* for(uint64_t i=0; i<cfg.elements; i++){
      if(MAP_CONTAINS(SET, i))
	numelem++;
	}*/
    //printf("Elements in map at end: %i \n", numelem);
    cout << "Elements in map at end: " << (numelem=PLIST_GETSIZE(LIST)) << endl;
    //printf("Thread added num + start elems: %i \n", numthread+numstart);
    cout << "Thread added num + start elems: " << (numthread+numstart) << endl;
    if((numthread+numstart) == numelem)
      printf("----------- Verification Succeeded -----------\n");
    else
      printf("++++++++++++++++++ Verifcation Failed ++++++++++++++++\n");
    PLIST_FREE(LIST);
    free(tls_array);
    MAIN_RETURN(0);

  }
