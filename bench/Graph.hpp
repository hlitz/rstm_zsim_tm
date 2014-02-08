/**
 *  Copyright (C) 2011
 *  University of Rochester Department of Computer Science
 *    and
 *  Lehigh University Department of Computer Science and Engineering
 *
 * License: Modified BSD
 *          Please see the file LICENSE.RSTM for licensing information
 */

#ifndef GRAPH_HPP__
#define GRAPH_HPP__
#include "List.hpp"
// We construct other data structures from the List. In order to do their
// sanity checks correctly, we might need to pass in a validation function of
// this type
typedef bool (*verifier)(uint32_t, uint32_t);

// Set of LLNodes represented as a linked list in sorted order
class Graph
{
  /*
  struct edge{
  uint64_t targetVertex;
  uint64_t* targetVertexPtr;
  edge(uint64_t _id, uint64_t* _ptr) : targetVertex(_id), targetVertexPtr(_ptr) {}
  }; */


  struct vertex
  {
    uint64_t id;
    List* edges;
    vertex(uint64_t _id) : id(_id) { 
      edges = (List*)hcmalloc(sizeof(List*));
      new (edges) List();
    }
    TM_CALLABLE
    bool insertEdge(uint64_t id, vertex* ptr TM_ARG){
      return edges->insert(id, (void*)ptr TM_PARAM);
    }
    TM_CALLABLE
    bool removeEdge(uint64_t id TM_ARG){
      return edges->remove(id TM_PARAM);
    }
    //     edges = malloc(sizeof(List));
    //  new (edges) List();
    //}
    //Node(int _key, List* _edges) : key(_key), edges(_edges) {}
  };

public:
  
  List* vertices;
 
  Graph();
  //Node nodes[MAX_NODES];
  
    // true iff val is in the data structure
    TM_CALLABLE
    bool lookupVertex(uint64_t vertexID, vertex** vert TM_ARG) const;

    TM_CALLABLE
    bool lookupVertex(uint64_t vertexID TM_ARG) const;

    // standard IntSet methods
    TM_CALLABLE
    bool insertVertex(uint64_t id, uint64_t* edges, uint64_t num_edges TM_ARG);

    // remove a node if its value = val
    TM_CALLABLE
    bool removeVertex(uint64_t val TM_ARG);

    // make sure the list is in sorted order
    bool isSane() const;
};

Graph::Graph(){
  vertices = (List*)hcmalloc(sizeof(List));
  //std::cout << "alloc " << sentinel << std::endl;
  new (vertices) List();
}


TM_CALLABLE
bool Graph::lookupVertex(uint64_t vertexID, vertex** vert TM_ARG) const
{
  void* v =  NULL;
  bool res = vertices->lookup(vertexID, &v TM_PARAM);
  if(res){
    assert(v);
    std::cout << (uint64_t)v << " dfd " << std::endl;
    *vert = (vertex*)v;
  }
  else
    vert = NULL;
  return res;
}


TM_CALLABLE
bool Graph::lookupVertex(uint64_t vertexID TM_ARG) const
{
  vertex* vert = NULL;
  bool res = Graph::lookupVertex(vertexID, &vert TM_PARAM);
  return res;
}

TM_CALLABLE
bool Graph::insertVertex(uint64_t id, uint64_t* edges, uint64_t num_edges TM_ARG)
{
  bool res = lookupVertex(id TM_PARAM);
  if(!res){ //vertex does not exist yet, we can insert
    vertex* v = NULL;
    v = (vertex*)hcmalloc(sizeof(vertex));
    new (v) vertex(id);
    vertices->insert(id, (void*)v TM_PARAM);
    for(uint64_t i =0; i< num_edges; i++){
      vertex* vv = NULL;
      bool res = lookupVertex(edges[i], &vv TM_PARAM);
      if(res){//node exists lets install the edge on both nodes
	v->insertEdge(edges[i], vv TM_PARAM);
	vv->insertEdge(id, v TM_PARAM);
      }    
    }
    return true;
  }
  return false;
}

TM_CALLABLE
bool Graph::removeVertex(uint64_t id TM_ARG){
  vertex* v = NULL;
  bool res = lookupVertex(id, &v TM_PARAM);
  std::cout << "1" << std::endl;
  if(res){ //vertex exists, lets delete it
    assert(v);
    List::list_iterator iter = v->edges->getFirst(TM_PARAM_ALONE);
    std::cout << "3" << std::endl;
    while(iter != NULL){
      vertex* remotev = (vertex*)iter->data;
      remotev->removeEdge(id TM_PARAM);
      iter = v->edges->getNext(iter TM_PARAM);
      std::cout << "4" << std::endl;
    }
    vertices->remove(id TM_PARAM);
    //we have removed ouself from all neighbours
    //hcfree(v);
  std::cout << "5" << std::endl;
    return true;
  }
  return false;
}

//bool Graph
  /*
    // make sure the list is in sorted order and for each node x,
    // v(x, verifier_param) is true
    bool extendedSanityCheck(verifier v, uint32_t param) const;

    // find max and min
    TM_CALLABLE
    int findmax(TM_ARG_ALONE) const;

    TM_CALLABLE
    int findmin(TM_ARG_ALONE) const;

    // overwrite all elements up to val
    TM_CALLABLE
    void overwrite(int val TM_ARG);
  */



// constructor just makes a sentinel for the data structure
//List::List() : sentinel(new Node()) { }


// simple sanity check: make sure all elements of the list are in sorted order
bool Graph::isSane(void) const
{
  return true;
}
/*
  bool correct = true;
  const Node* prev(sentinel);
  const Node* curr((prev->m_next));
  int elems = 0;
  while (curr != NULL) {
    if ((prev->m_val) >= (curr->m_val))
      correct = false;
    prev = curr;
    curr = curr->m_next;
    elems++;
  }
  std::cout << "Elements in the List at End: " << elems << std::endl;
  const Node* prev2(sentinel);
  const Node* curr2((prev2->m_next));

  if(!correct){
    while (curr2 != NULL) {
      std::cout << prev2->m_val<< " >= " <<curr2->m_val << std::endl;
      prev2 = curr2;
      curr2 = curr2->m_next;
      
    }
    return false;
  }
  return true;
}

// extended sanity check, does the same as the above method, but also calls v()
// on every item in the list
bool List::extendedSanityCheck(verifier v, uint32_t v_param) const
{
    const Node* prev(sentinel);
    const Node* curr((prev->m_next));
    while (curr != NULL) {
        if (!v((curr->m_val), v_param) || ((prev->m_val) >= (curr->m_val)))
            return false;
        prev = curr;
        curr = prev->m_next;
    }
    return true;
}

// insert method; find the right place in the list, add val so that it is in
// sorted order; if val is already in the list, exit without inserting
TM_CALLABLE
bool List::insert(int val TM_ARG)
{
    // traverse the list to find the insertion point
    const Node* prev(sentinel);
    const Node* curr(TM_READ(prev->m_next));
    //printf("start insert\n");
    while (curr != NULL) {
        if (TM_READ(curr->m_val) >= val)
            break;
        prev = curr;
        curr = TM_READ(prev->m_next);
    }

    // now insert new_node between prev and curr
    if (!curr || (TM_READ(curr->m_val) > val)) {
        Node* insert_point = const_cast<Node*>(prev);

        // create the new node
        Node* i = (Node*)TM_ALLOC(sizeof(Node));
	//std::cout << "alloc ins " << i << std::endl;
        i->m_val = val;
        i->m_next = const_cast<Node*>(curr);
        TM_WRITE(insert_point->m_next, i);
	//std::cout << "now return " << std::endl;
	return true;
    }
    else
      return false;
}

// search function
TM_CALLABLE
bool List::lookup(int val TM_ARG) const
{
    bool found = false;
    const Node* curr(sentinel);
    curr = TM_READ(curr->m_next);

    while (curr != NULL) {
        if (TM_READ(curr->m_val) >= val)
            break;
        curr = TM_READ(curr->m_next);
    }

    found = ((curr != NULL) && (TM_READ(curr->m_val) == val));
    return found;
}

// findmax function
TM_CALLABLE
int List::findmax(TM_ARG_ALONE) const
{
    int max = -1;
    const Node* curr(sentinel);
    while (curr != NULL) {
        max = TM_READ(curr->m_val);
        curr = TM_READ(curr->m_next);
    }
    return max;
}

// findmin function
TM_CALLABLE
int List::findmin(TM_ARG_ALONE) const
{
    int min = -1;
    const Node* curr(sentinel);
    curr = TM_READ(curr->m_next);
    if (curr != NULL)
        min = TM_READ(curr->m_val);
    return min;
}

// remove a node if its value == val
TM_CALLABLE
bool List::remove(uint64_t val TM_ARG)
{
    // find the node whose val matches the request
    const Node* prev(sentinel);
    const Node* curr(TM_READ(prev->m_next));
    
    while (curr != NULL) {
        // if we find the node, disconnect it and end the search
        if (TM_READ(curr->m_val) == val) {
            Node* mod_point = const_cast<Node*>(prev);
            TM_WRITE(mod_point->m_next, TM_READ(curr->m_next));
	    TM_WRITE(((Node*)curr)->m_next, (Node*)NULL); //dummy write
            // delete curr...
            TM_FREE(const_cast<Node*>(curr));
	    return true;
            break;
        }
        else if (TM_READ(curr->m_val) > val) {
            // this means the search failed
	  return false;
            break;
        }
        prev = curr;
        curr = TM_READ(prev->m_next);
    }
    return false;
}

// search function
TM_CALLABLE
void List::overwrite(int val TM_ARG)
{
    const Node* curr(sentinel);
    curr = TM_READ(curr->m_next);

    while (curr != NULL) {
        if (TM_READ(curr->m_val) >= val)
            break;
        Node* wcurr = const_cast<Node*>(curr);
        TM_WRITE(wcurr->m_val, TM_READ(wcurr->m_val));
        curr = TM_READ(wcurr->m_next);
    }
    }*/







#endif // LIST_HPP__
