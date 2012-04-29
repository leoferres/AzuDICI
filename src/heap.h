#ifndef _HEAP_H_
#define _HEAP_H_

#include <stdio.h>
#include "kvec.h"
#include <stdbool.h>
#include "common.h"
#include <float.h>

typedef struct _maxheap {
	kvec_t(unsigned int) maxHeap;
	kvec_t(double) act;
	kvec_t(unsigned int) heapPositions; // position in heap of each var (0 if not in maxHeap)
	unsigned int maxHeapLast; //zero iff heap is empty   
	unsigned int numElems;
} MaxHeap;

void maxHeap_init(MaxHeap *mh,unsigned int nElems);
void heap_placeNode(int n, unsigned int pos, MaxHeap *mh);
void heap_percolateUp(unsigned int pos, MaxHeap *mh);
bool heap_nodeIsGreater (int n1, int n2, MaxHeap *mh);
void heap_normalizeScores(MaxHeap *mh);
unsigned int heap_consultMax(MaxHeap *mh); //returns 0 if empty
unsigned int maxHeap_remove_max(MaxHeap *mh);
void maxHeap_insert_element(MaxHeap *mh, unsigned int elem); //x may already be in the heap
bool maxHeap_increase_score_in(MaxHeap *mh, unsigned int elem, double val); //val needs to be greater than current. Returns true if normalization has taken place.
void heap_resetKeepingValues(MaxHeap *mh); //everybody again in the heap with val 0
unsigned int heap_addNewElement(double val, MaxHeap *mh); //adds new element to the heap with value
bool heap_isCorrect(MaxHeap *mh);

#endif /* _HEAP_H_ */
