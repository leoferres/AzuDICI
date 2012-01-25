#ifndef _MODEL_H_
#define _MODEL_H_

#include "stack.h" /*TO DO*/
#include "varinfo.h"
#include "heap.h" /*TO DO*/

Stack model; /* Keeps track of the state of the model */ 
int last2propagated, last3propagated, lastNpropagated;
char* assignment; /*truth value of each literal*/
unsigned int decision_lvl;
VarInfo* vinfo; /*keeps track of each variable*/
Heap maxheap;

void set_true_due_to_decision(Literal l);
void set_true_due_to_reason(Literal l, Reason r);

#endif /* _MODEL_H_ */




