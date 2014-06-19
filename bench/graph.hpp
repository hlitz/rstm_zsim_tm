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

#include "list.hpp"
// We construct other data structures from the List. In order to do their
// sanity checks correctly, we might need to pass in a validation function of
// this type
typedef bool (*verifier)(uint32_t, uint32_t);

// Set of LLNodes represented as a linked list in sorted order
class Graph
{

  List* vertices;

  struct edge{
    uint64_t targetVertex;
    uint64_t* targetVertexPtr;
    edge(uint64_t _id, uint64_t* _ptr) : id(_id), ptr(_ptr) {}
  } 


  struct vertex
  {
    uint64_t id;
    List* edges;
    vertex(uint64_t _key, List* _edges) : key(_key), edges(_edges) { }
    bool insertEdge(uint64_t id, uint64_t* ptr){
      return edges->insert(id, ptr);
    };
    bool removeEdge(uint64_t id){
      return edges->remove(id);
    };
    //     edges = hcmalloc(sizeof(List));
    //  new (edges) List();
    //}
    //Node(int _key, List* _edges) : key(_key), edges(_edges) {}
  };

  public:
  
  
  Graph();
  //Node nodes[MAX_NODES];
  
    // true iff val is in the data structure
    TM_CALLABLE
    bool lookupVertex(uint64_t vertexID, vertex* vert TM_ARG) const;

    TM_CALLABLE
    bool lookupVertex(uint64_t vertexID) const;

    // standard IntSet methods
    TM_CALLABLE
    bool insertVertex(uint64_t id, uint64_t* edges, uint64_t num_edges TM_ARG);

    // remove a node if its value = val
    TM_CALLABLE
    bool removeVertex(uin64_t val TM_ARG);

    // make sure the list is in sorted order
    bool isSane() const;
};

Graph::Graph(){
  vertices = (List*)hcmalloc(sizeof(List));
  //std::cout << "alloc " << sentinel << std::endl;
  new (vertices) List();
}

bool Graph::lookupVertex(uint64_t vertexID){
  vertex* vert;
  return lookupVertex(vertexID, vert);
}

bool Graph::lookupVertex(uint64_t vertexID, vertex* vert TM_ARG){
  vertex* v;
  bool res = vertices->lookup(nodeID, (void*)v);
  if(res){
    vert = v;
  }
  else
    vert = NULL;
  return res;
}

bool Graph::insertVertex(uint64_t id, uint64_t* edges, uint64_t num_edges){
  vertex* v;
  bool res = lookupVertex(id, v);
  if(!res){ //vertex does not exist yet, we can insert
    v = (vertex*)hcmalloc(sizeof(vertex));
    new (v) vertex(id);
    vertices->insert(id, (void*)v);
    for(uint64_t i =0; i< num_edges; i++){
      vertex* vv;
      bool res = lookupVertex(edges[i], vv);
      if(res){//node exists lets install the edge on both nodes
	v->insertEdge(edges[i], vv);
	vv->insertEdge(id, v);
      }    
    }
    return true;
  }
  return false;
}

bool Graph::deleteVertex(uint64_t id){
  vertex* v;
  bool res = lookupVertex(id, v);
  if(res){ //vertex exists, lets delete it
    Node* node = v->edges->getFirst();
    while(node != NULL){
      vertex* remotev = (vertex*)node->data;
      remotev->removeEdge(id);
      node = v->edges->getNext(node);
    }
    //we have removed ouself from all neighbours
    hcfree(v);
  }
}

bool Graph
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
Graph::Graph(){
  sentinel = (Graph*)hcmalloc(sizeof(Graph));
  //std::cout << "alloc " << sentinel << std::endl;
  new (sentinel) Graph();
}


// simple sanity check: make sure all elements of the list are in sorted order
bool List::isSane(void) const
{
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
bool List::remove(int val TM_ARG)
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
}

#endif // LIST_HPP__
