#include "heap.h"

/*The constructor of the MaxHeap structure*/
void maxHeap_init(MaxHeap *mh, unsigned int nElems){
       
    /*We initialize and set the size of the maxHeap vector*/
    kv_init(mh->maxHeap); 
    kv_resize(unsigned int,mh->maxHeap,nElems+1);
    kv_size(mh->maxHeap)=nElems+1;
    
    /*We initialize and set the size of the act vector*/
    kv_init(mh->act);
    kv_resize(double,mh->act,nElems+1);
    kv_size(mh->act)=nElems+1;
    
    /*We initialize and set the size of the heaPositions vector*/        
    kv_init(mh->heapPositions);
    kv_resize(unsigned int,mh->heapPositions,nElems+1);
    kv_size(mh->heapPositions)=nElems+1;
    
    /*Initialize variables*/
    mh->maxHeapLast=0;
    mh->numElems=nElems;
    
    /*Set first element of maxHeap to 0*/
    kv_A(mh->maxHeap,0)=0;
    
    /*Set rest of elements of vectors*/
    for(unsigned int elem=1;elem<=nElems;elem++){
        kv_A(mh->maxHeap,elem)=elem;
        kv_A(mh->act,elem)=0;
        kv_A(mh->heapPositions,elem)=elem;
    }
    
    mh->maxHeapLast = nElems;
}

/*Returns the highest element in the heap. If empty returns 0.*/
unsigned int heap_consultMax(MaxHeap *mh){
    if(!mh->maxHeapLast) return 0;
    else return kv_A(mh->maxHeap,1);
}

unsigned int maxHeap_remove_max(MaxHeap *mh){
    unsigned int resultVar;
    unsigned int pos=1, childPos=2;
    int node, childNode, rchildNode;
    if(!mh->maxHeapLast) return 0; // Heap is empty
    resultVar = kv_A(mh->maxHeap,1);
    kv_A(mh->heapPositions,resultVar)=0; // out of heap
    node = kv_A(mh->maxHeap,mh->maxHeapLast--);
    /* now sink node until its place, i.e., while lchild exists:*/
    while(childPos <= mh->maxHeapLast){
        childNode = kv_A(mh->maxHeap,childPos);
        if(childPos < mh->maxHeapLast){
            rchildNode = kv_A(mh->maxHeap,childPos+1);
            /*if rchild also exists, make childNode the largest of both:*/
            if(heap_nodeIsGreater(rchildNode,childNode,mh)){
                childNode=rchildNode;
                childPos++;
            }
        }
        if(heap_nodeIsGreater(node,childNode,mh)) break; // no need to sink any further
        heap_placeNode(childNode,pos,mh);
        pos = childPos;
        childPos = pos*2;
    }
    if(mh->maxHeapLast) heap_placeNode(node,pos,mh);
    dassert(kv_A(mh->heapPositions,resultVar)==0);
    return(resultVar);
}

unsigned int heap_addNewElement(double val,MaxHeap *mh){
    mh->numElems++;
    /*mh.maxHeap.push();*/
    kv_push(double,mh->act,val);
    kv_push(unsigned int,mh->heapPositions,0);
    dassert(kv_A(mh->heapPositions,mh->numElems)==0);
    maxHeap_insert_element(mh,mh->numElems);
    return mh->numElems;
}

bool heap_nodeIsGreater(int n1, int n2, MaxHeap *mh){
    return (kv_A(mh->act,n1)>kv_A(mh->act,n2) || (kv_A(mh->act,n1)==kv_A(mh->act,n2) && kv_A(mh->act,n1)>kv_A(mh->act,n2)));
}

void heap_resetKeepingValues(MaxHeap *mh){
    for(unsigned int elem=1;elem<=mh->numElems;elem++){
        if(kv_A(mh->heapPositions,elem)==0) maxHeap_insert_element(mh,elem);
    }
}

void heap_resetAllValuesToZero(MaxHeap *mh){
    for(unsigned int elem=1;elem<=mh->numElems;elem++){
        kv_A(mh->maxHeap,elem)=elem;
        kv_A(mh->act,elem)=0;
        kv_A(mh->heapPositions,elem)=elem;
    }
    mh->maxHeapLast=mh->numElems;
}

bool heap_isCorrect(MaxHeap *mh){
    for(unsigned int i=1;i<=mh->maxHeapLast;i++){
        if(2*i<=mh->maxHeapLast){
            if(kv_A(mh->act,kv_A(mh->maxHeap,i)) < kv_A(mh->act,kv_A(mh->maxHeap,2*i))){
                printf("Problem between pos %i and %i\n",i,2*i);
                printf("They have activities %f and %f\n",kv_A(mh->act,kv_A(mh->maxHeap,i)),kv_A(mh->act,kv_A(mh->maxHeap,2*i)));
                return false;
            }
        }
        if(2*i+1<mh->maxHeapLast){
            if(kv_A(mh->act,kv_A(mh->maxHeap,i)) < kv_A(mh->act,kv_A(mh->maxHeap,2*i+1))){
                printf("Problem between pos %i and %i\n",i,2*i+1);
                printf("They have activities %f and %f\n",kv_A(mh->act,kv_A(mh->maxHeap,i)),kv_A(mh->act,kv_A(mh->maxHeap,2*i+1)));
                return false;
            }
        }
    }
    return true;
}

void heap_percolateUp(unsigned int pos, MaxHeap *mh){
    int node = kv_A(mh->maxHeap,pos), parentNode;
    while(pos > 1){ //not yet at top of the maxHeap
        parentNode = kv_A(mh->maxHeap,pos/2);
        if(heap_nodeIsGreater(parentNode,node,mh)) break;
        heap_placeNode(parentNode,pos,mh);
        pos=pos/2;
    }
    heap_placeNode(node,pos,mh);
}

void maxHeap_insert_element(MaxHeap *mh, unsigned int elem){
    if(kv_A(mh->heapPositions,elem)!=0) return;
    unsigned int pos = ++mh->maxHeapLast;
    kv_A(mh->maxHeap,pos)=elem;
    heap_percolateUp(pos,mh);
}

void heap_normalizeScores(MaxHeap *mh){
    static int nTimes=0;
    nTimes++;
    double score;
    for(unsigned int elem=1;elem<mh->numElems;elem++){
        score = kv_A(mh->act,elem) /= (DBL_MAX/20);
        if(score < DBL_MIN) kv_A(mh->act,elem)=0; //underflow
    }
}

bool maxHeap_increase_score_in (MaxHeap *mh, unsigned int elem, double val){
    bool toReturn = false;
    dassert(val>=0);
    double newScore = kv_A(mh->act,elem)+val;
    if(newScore > DBL_MAX){ // i.e., newScore is "infty": overflow.
        heap_normalizeScores(mh);
        toReturn = true;
    }
    kv_A(mh->act,elem)+=val;
    unsigned int pos = kv_A(mh->heapPositions,elem);
    if(pos) heap_percolateUp(pos,mh); //if in heap percolate up
    return toReturn;
}

void heap_placeNode(int n, unsigned int pos, MaxHeap *mh){
    kv_A(mh->maxHeap,pos)=n;
    kv_A(mh->heapPositions,n)=pos;
}
