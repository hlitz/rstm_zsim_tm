/**
 *  Copyright (C) 2011
 *  University of Rochester Department of Computer Science
 *    and
 *  Lehigh University Department of Computer Science and Engineering
 *
 * License: Modified BSD
 *          Please see the file LICENSE.RSTM for licensing information
 */

#ifndef HASH_HPP__
#define HASH_HPP__

//#include "List.hpp"

// the Hash class is an array of N_BUCKETS LinkedLists
class HashTable
{
  static const uint32_t N_SLOTS = 10000;
  uint32_t N_BUCKETS;

    /**
     *  during a sanity check, we want to make sure that every element in a
     *  bucket actually hashes to that bucket; we do it by passing this
     *  method to the extendedSanityCheck for the bucket.
     */
    static bool verify_hash_function(uint32_t val, uint32_t bucket)
    {
      return true;//((val % N_BUCKETS) == bucket);
    }

  public:
    /**
     *  Templated type defines what kind of list we'll use at each bucket.
     */

    int ** bucket;
    uint32_t * bucket_entries;
 
    TM_CALLABLE
    void init(uint32_t _buckets)
  {
    N_BUCKETS = _buckets;
    bucket = (int **)hcmalloc(sizeof(uint32_t*)*N_BUCKETS);
    bucket_entries = (uint32_t*)hcmalloc(sizeof(uint32_t)*N_BUCKETS);
    for(uint32_t i =0; i< N_BUCKETS; i++){
      bucket_entries[i] = 0;
    }
    for(uint32_t i =0; i< N_BUCKETS; i++){
      bucket[i] = (int*)hcmalloc(sizeof(uint32_t)*N_SLOTS);
    }
  }

    TM_CALLABLE
    void insert(int val TM_ARG)
    {
      //std::cout << " n buck " << N_BUCKETS << std::endl;
      uint32_t buck = val % N_BUCKETS;
      uint32_t slots = TM_READ(bucket_entries[buck])+1;
      TM_WRITE(bucket_entries[buck], slots);
      TM_WRITE(bucket[buck][slots], val);
    }

    TM_CALLABLE
    bool lookup(int val TM_ARG) const
    {
      bool found =false;
      uint32_t buck = val % N_BUCKETS;
      uint32_t slots = TM_READ(bucket_entries[buck]);
      for(uint32_t i = 0; i<slots ; i++){
	if(val==TM_READ(bucket[buck][i])) found =true;
      }
      return found;
    }

    TM_CALLABLE
    void remove(int val TM_ARG)
    {
      //std::cout << " val " << val << std::endl;
      //bool found = false;
      uint32_t buck = val % N_BUCKETS;
      //std::cout << "cuck " << buck << std::endl;
      
      uint32_t slots = TM_READ(bucket_entries[buck]);
      //std::cout << slots << " slots " << std::endl;
      if(slots>0){
	for(uint32_t i = 0; i<slots ; i++){
	  if(val==TM_READ(bucket[buck][i]) && val !=0 ){
	    //TM_WRITE(bucket[buck][i], 0);
	    TM_WRITE(bucket_entries[buck], TM_READ(bucket_entries[buck])-1);
	    //found = true;
	    for(uint32_t e = i; e<slots; e++){
	      TM_WRITE(bucket[buck][e], TM_READ(bucket[buck][e+1])); //shift the remainder elements to the left
	    }
	    break;
	  }
	}
      }
      //if(!found) std::cout << "not present" << val << std::endl;
      //else std::cout << "PRESENT" << val << std::endl;
     
    }

    bool isSane() const
    {/*
        for (int i = 0; i < N_BUCKETS; i++)
            if (!bucket[i].extendedSanityCheck(verify_hash_function, i))
	    return false;*/
        return true;
    }
};

#endif // HASH_HPP__
