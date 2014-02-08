/**
 *  Copyright (C) 2011
 *  University of Rochester Department of Computer Science
 *    and
 *  Lehigh University Department of Computer Science and Engineering
 *
 * License: Modified BSD
 *          Please see the file LICENSE.RSTM for licensing information
 */

#ifndef GRAPHMATRIX_HPP__
#define GRAPHMATRIX_HPP__
#include "List.hpp"

typedef bool (*verifier)(uint32_t, uint32_t);

class GraphMatrix
{

  struct row{
    uint64_t* columns;
  };
  row* rows;
   
  //  uint64_t adjacencyMatrix[][];
  uint64_t* vertices;

  uint64_t max_vertices;

  TM_CALLABLE
  bool insertEdge(uint64_t from, uint64_t to TM_ARG);

  TM_CALLABLE
  bool removeEdge(uint64_t from, uint64_t to TM_ARG);
  
  TM_CALLABLE
  bool lookupEdge(uint64_t from, uint64_t to TM_ARG);

public:

  GraphMatrix();
  GraphMatrix(uint64_t _max_vertices);
  
  TM_CALLABLE
  uint64_t insertVertex(uint64_t id, uint64_t* edges, uint64_t num_edges TM_ARG);

  TM_CALLABLE
  uint64_t removeVertex(uint64_t id TM_ARG);

  TM_CALLABLE
  bool lookupVertex(uint64_t id TM_ARG);

  bool isSane() const;

  TM_CALLABLE
  uint64_t pageRank(uint64_t id, uint64_t level TM_ARG);
  
  TM_CALLABLE
  bool removeLargest( TM_ARG_ALONE);
};

GraphMatrix::GraphMatrix(){
  GraphMatrix(100);
}

GraphMatrix::GraphMatrix(uint64_t _max_vertices){
  max_vertices = _max_vertices;
  vertices = (uint64_t*)hcmalloc(sizeof(uint64_t)*max_vertices);
  rows = (row*)hcmalloc(sizeof(row)*max_vertices);
  std::cout << "vert " << max_vertices << std::endl;
  for(uint i =0; i< max_vertices; i++){
    rows[i].columns = (uint64_t*)hcmalloc(sizeof(uint64_t)*max_vertices);
    vertices[i] = 0;
    for(uint e = 0; e < max_vertices; e++){
      rows[i].columns[e] = 0;
    }
  }
}

bool GraphMatrix::isSane() const{
  uint64_t sum = 0;
  for(uint64_t e = 0; e< max_vertices; e++){
    for(uint64_t i = 0; i< max_vertices; i++){
      sum += rows[i].columns[e];
    }
  }
  std::cout << "Sum of edges in the matrix " << sum/2 << std::endl;
  return true;
}

TM_CALLABLE
bool GraphMatrix::insertEdge(uint64_t from, uint64_t to TM_ARG){//* edges, uint64_t num_edges TM_ARG)
  if(from == to)
    return false;
  if(TM_READ(vertices[from]) && TM_READ(vertices[to]) && !TM_READ(rows[from].columns[to])){ //We only need the 3rd term such that return val is correct
    uint64_t set = 1;
    assert(TM_READ(rows[from].columns[to]) == TM_READ(rows[to].columns[from]));
    TM_WRITE(rows[from].columns[to], set);
    TM_WRITE(rows[to].columns[from], set);
    return true;
  }
  return false;
}

TM_CALLABLE
bool GraphMatrix::removeEdge(uint64_t from, uint64_t to TM_ARG){//* edges, uint64_t num_edges TM_ARG)
  if(from == to)
    return false;
  if(TM_READ(rows[from].columns[to])){//vertices[from]) && TM_READ(vertices[to])){
    uint64_t unset = 0;
    assert(TM_READ(rows[from].columns[to]) == TM_READ(rows[to].columns[from]));
    TM_WRITE(rows[from].columns[to], unset);
    TM_WRITE(rows[to].columns[from], unset);
    return true;
  }
  return false;
}

TM_CALLABLE
bool GraphMatrix::lookupEdge(uint64_t from, uint64_t to TM_ARG){//* edges, uint64_t num_edges TM_ARG)
  return (TM_READ(vertices[from]) && TM_READ(vertices[to])); 
}

TM_CALLABLE
uint64_t GraphMatrix::insertVertex(uint64_t id, uint64_t* edges, uint64_t num_edges TM_ARG)
{
  uint64_t insertedEdges = 0;
  if(!TM_READ(vertices[id])){
    uint64_t set = 1;
    TM_WRITE(vertices[id], set);
    for(uint i =0; i<num_edges; i++){
      bool res = insertEdge(id, edges[i] TM_PARAM);
      if(res) insertedEdges++;
    }
  }
  return insertedEdges;
}

TM_CALLABLE
uint64_t GraphMatrix::removeVertex(uint64_t id TM_ARG)
{
  uint64_t removedEdges = 0;
  uint64_t exists = TM_READ(vertices[id]);
  if(exists){
    uint64_t unset =0;
    TM_WRITE(vertices[id], unset);
    for(uint i =0; i<max_vertices; i++){
      bool res = removeEdge(id, i TM_PARAM);
      if(res) removedEdges++;
    }
  }
  return removedEdges;
}

TM_CALLABLE
bool GraphMatrix::lookupVertex(uint64_t id TM_ARG)
{
  return TM_READ(vertices[id]);
}

TM_CALLABLE
uint64_t GraphMatrix::pageRank(uint64_t id, uint64_t level TM_ARG){
  uint64_t sum = 0;
  for(uint e=0; e<max_vertices; e++){
    if(lookupEdge(id, e TM_PARAM)){
      if(level==1){
	sum += 1;
      }
      else{
	uint64_t shift = level;
	sum += (pageRank(e, --level TM_PARAM)<<shift);
      }
    }
  }
  return sum;
}


TM_CALLABLE
bool GraphMatrix::removeLargest( TM_ARG_ALONE){
  uint64_t level = 1; //recursive depth of pageRank algo
  uint64_t* results = (uint64_t*)hcmalloc(sizeof(uint64_t)*max_vertices);
  for(uint i=0; i<max_vertices; i++){
    if(lookupVertex(i TM_PARAM)==0){
      results[i] = 0;
    }
    else{
      results[i] = pageRank(i, level TM_PARAM);
    }
  }
  uint64_t largest =0;
  for(uint i=0; i<max_vertices; i++){
    if(results[i]>largest){
      largest = results[i];
    }
  }
  //  removeVertex(largest TM_PARAM);
  //      std::cout << "rank : " << largest << std::endl;
  return true;
}

#endif // LIST_HPP__
