#ifndef _CLAUSEDB_C_
#define _CLAUSEDB_C_

#include "clausedb.h"

Clause inputClause;
unsigned int nInsertedClauses;
pthread_rwlock_t insert_unitary_clause_lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t insert_binary_clause_lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t insert_ternary_clause_lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t insert_nary_clause_lock = PTHREAD_RWLOCK_INITIALIZER;

ClauseDB* init_clause_database(unsigned int nVars, unsigned int nWorkers){

  kv_init(inputClause.lits); //init inputClause
  kv_resize(Literal, inputClause.lits, nVars);//Set size of nVars

  inputClause.size = 0; //the vector is nVars size, so we keep the number of lits actually added in other counter.
  nInsertedClauses=0;

  ClauseDB* cdb = (ClauseDB*)malloc(sizeof(ClauseDB));

  cdb->numVars         = nVars;
  cdb->numWorkers      = nWorkers;

  cdb->numClauses      = 0;
  cdb->numUnits        = 0;
  cdb->numBinaries     = 0;
  cdb->numTernaries    = 0;
  cdb->numNClauses     = 0;
  cdb->numInputClauses = 0;

  /*Init Unitary clauses database*/
  ts_vec_init(cdb->uDB); //init vector with 0 elements
  /******************************/

  /*Init Binary clauses database*/
  kv_init(cdb->bDB);//init vector 
  kv_resize(ts_vec_t(Literal), cdb->bDB, 2*(nVars+1) ); //We know size is fixed to 2*(nVars+1) elements

  /*Init each of the binary clause database elements*/
  for(int i=0;i<=2*(nVars+1);i++){
    ts_vec_init(kv_A(cdb->bDB,i));
  }
  /******************************/

  /*Init Ternary clauses database*/
  ts_vec_init(cdb->tDB); //init vector with 0 elements
  /*******************************/

  /*******************************/
  ts_vec_init(cdb->nDB); //init vector with 0 elements
  /*******************************/

  /*Init 3watches structures*/
  kv_init( cdb->ternaryWatches );
  kv_resize(ts_vec_t(unsigned int), cdb->ternaryWatches,  2*(nVars+1) );
  for(int i=0;i<=2*(nVars+1);i++){
    ts_vec_init( kv_A(cdb->ternaryWatches,i) );
  }
  /*************************/


  /*Init random numbers vector*/
  srandom(0);
  for(int i=0;i<MAX_RANDOM_NUMBERS;i++){
    cdb->randomNumbers[i] = random();
  }
  /****************************/

  return cdb;
}

/*Insertion sort for normal clauses, given a size*/
/*
void vec_literal_sort(Clause* cl, unsigned int size){
  int i,j;
  Literal aux;
  for( i=0; i < size-1; i++ ){
    for( j = i+1; j < size; j++ ){
      if( kv_A(cl->lits,i) > kv_A(cl->lits,j)){
	aux = kv_A(cl->lits,i);
	kv_A(cl->lits,i) = kv_A(cl->lits,j);
	kv_A(cl->lits,j) = aux;
      }
    }
  }
}*/

/*Adding an input literal (from file) to the input clause*/
unsigned int add_input_literal(ClauseDB* cdb, Literal l){
  /*checking if the clause is complete.
   A literal with value 0 indicates this.*/
    if(l==0){
    switch(inputClause.size){
    case 0:
      return 20; //empty clause in original formula
    case 1:
      insert_unitary_clause(cdb, &inputClause, true);
      break;
    case 2:
      insert_binary_clause(cdb, &inputClause, true);
      break;
    case 3:
      insert_ternary_clause(cdb, &inputClause,true,0);
      break;
    default:
      insert_nary_clause(cdb, &inputClause, true,0);
    }    
    //new clause will be read
    inputClause.size = 0;
  }else{
        /*if the clause is not complete, we add the literal to it*/
    kv_A(inputClause.lits,inputClause.size) = l;
    inputClause.size++;
  }
}

/*Inserting, in the clause database, an input clause that only has one literal*/
void insert_unitary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal){
  pthread_rwlock_wrlock(&insert_unitary_clause_lock);
  dassert(cl->size == 1);
  int i;
  bool alreadyAdded=false;
  Literal unitInList;
  int listSize;
  ts_vec_size(listSize, cdb->uDB);
  dassert(listSize == cdb->numUnits);
/*Checking if the literal is already in the uDB (unitary clause database).*/
  for(i=0; i < cdb->numUnits; i++){
    ts_vec_ith(unitInList, cdb->uDB, i);
    if( kv_A(cl->lits,0) == unitInList ) {
      alreadyAdded=true;
      break;
    }
  }
  /*If it isn't already in the uDB, add it*/
  if(!alreadyAdded){
    ts_vec_push_back( Literal, cdb->uDB, kv_A(cl->lits,0) );
    if(isOriginal) cdb->numInputClauses++;
    cdb->numUnits++;
    cdb->numClauses++;
  }
  pthread_rwlock_unlock(&insert_unitary_clause_lock);
}


/*******************Make this Thread safe****************************/
  /*bDB stores 2 literals in a clause as following:
   dDB is a vector which size is 2*(numVars+1) and each position in the vector
   corresponds to a literal which has a literal vector associated to it. If the 
   clause (x1 or ~x2) is to be stored in dBD, we store x1 in the literal vector
   associated to the literal x2, and we store ~x2 in the literal vector associated
   to literal ~x1. This way we have an implication vector for all literals. 
   Associating x1 with x2 means that if x2 were to be true, then x1 must also be true.
   Associating ~x2 with ~x1 means that if ~x1 were to be true, then ~x2 must also be true*/
void insert_binary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal){
  pthread_rwlock_wrlock(&insert_binary_clause_lock);
  //we will have problems here for keeping the same search for each thread
  int i;
  dassert(cl->size == 2);
  /*We are storing implications, so we want to know the negation of each literal
   that belongs to the binary clause*/
  Literal l1,l2;
  l1 = kv_A(cl->lits,0);
  l2 = kv_A(cl->lits,1);
  unsigned int not_l1,not_l2;
  not_l1 = lit_as_uint(-l1);
  not_l2 = lit_as_uint(-l2);

  /*Search to see if element is already in list*/
  bool alreadyInList=false;
  Literal lInList;  
  int listSize;
  ts_vec_size(listSize, kv_A(cdb->bDB, not_l1));
  for(i=0;i<listSize;i++){
    ts_vec_ith(lInList,kv_A(cdb->bDB, not_l1),i);
    if(lInList == l2){
      alreadyInList=true;
    }
  }
  /*********************************************/

  /*Insert and update, if not previously inserted*/
  if(!alreadyInList){
    ts_vec_push_back(Literal,kv_A(cdb->bDB, not_l1),l2);
    //If it is not in list, it's not in the other list either
    ts_vec_push_back(Literal, kv_A(cdb->bDB, not_l2),l1);
    //update clauseDB stats
    if(isOriginal) cdb->numInputClauses++;
    cdb->numBinaries++;
    cdb->numClauses++;
  }
  /********************************************/
  pthread_rwlock_unlock(&insert_binary_clause_lock);
}

/*Function for sorting literals in a vector.
 Sorts from lower to highest*/

void vec_literal_sort(Literal *lits, unsigned int size) {

    struct sort_node *head;
    struct sort_node *current;
    struct sort_node *next;
    int i = 0;

    kvec_t(Literal) tmp;
    tmp.a = lits;

    head = NULL;
    /* insert the numbers into the linked list */
    for (i = 0; i < size; i++)
        head = addsort_node(kv_A(tmp, i), head);

    /* sort the list */
    head = mergesort(head);

    /*Store the list back into the vector*/
    i = 0;
    for (current = head; current != NULL; current = current->next)
        kv_A(tmp, i++) = current->number;

    /* free the list */
    for (current = head; current != NULL; current = next)
        next = current->next, free(current);
}


/************MAKE THIS THREAD SAFE**********/
/* To insert a clause with 3 literals into the tDB*/
Literal* insert_ternary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, int wId) {
    pthread_rwlock_wrlock(&insert_ternary_clause_lock);
    dassert(cl->size == 3);
    int i, j;
    int index = -1;
    TClause ternary;

    vec_literal_sort(cl->lits.a, cl->size);

    /*Check if alreadyAdded*/
    int listSize;
    ts_vec_size(listSize, cdb->tDB);
    dassert(cdb->numTernaries == listSize);
    for (i = 0; i < cdb->numTernaries; i++) {
        ts_vec_ith(ternary, cdb->tDB, i);
        index = i;
        for (j = 0; j < 3; j++) {
            if (ternary.lits[j] != kv_A(cl->lits, j)) {
                index = -1;
                break;
            }
        }
        if (index != -1) break;
    }
    /*********************/

    /*In case the clause already exists, modify flags if it's learned*/
    if (index != -1) {
        TClause *tmpPtr;
        dassert(index >= 0);
        if (!isOriginal) {
            ts_vec_ith(ternary, cdb->tDB, index);
            ternary.flags[wId] = true;
            ts_vec_set_ith(TClause,cdb->tDB,index,ternary);            
            ts_vec_ith_ma(tmpPtr,cdb->tDB,index);
        }
        /*Return a pointer to the actual ternary inserted clause*/
        return &tmpPtr->lits[0];
    }
    /********************************************************/

    /*In case it doesn't exist, insert it and watch*/
    //Init flags

    if (isOriginal) {
        for (i = 0; i < cdb->numWorkers; i++) {
            ternary.flags[i] = true;
        }
    } else {
        for (i = 0; i < cdb->numWorkers; i++) {
            if (i == wId) ternary.flags[i] = true;
            else ternary.flags[i] = false;
        }
    }

    //Insert literals
    for (i = 0; i < 3; i++) {
        ternary.lits[i] = kv_A(cl->lits, i);
    }
    //update clauseDB
    ts_vec_push_back(TClause, cdb->tDB, ternary);

    /*add this clause position in tDB to each neg literal watches*/
    unsigned int litIndex;
    unsigned int indexOfNewClause;
    ts_vec_size(indexOfNewClause, cdb->tDB);
    indexOfNewClause--;

    for (i = 0; i < 3; i++) {
        litIndex = lit_as_uint(-ternary.lits[i]);
        ts_vec_push_back(unsigned int, kv_A(cdb->ternaryWatches, litIndex), indexOfNewClause);
    }

    if (isOriginal) cdb->numInputClauses++;
    cdb->numTernaries++;
    cdb->numClauses++;
    /*Return a pointer to the actual ternary inserted clause*/
    TClause *tmpPtr;         
    ts_vec_ith_ma(tmpPtr,cdb->tDB,indexOfNewClause);
    return &tmpPtr->lits[0];
    //  dassert(ts_vec_size(TClause, cdb->tDB)==cdb->numTernaries);
    /************************************************/
    pthread_rwlock_unlock(&insert_ternary_clause_lock);
}


/****MAKE THIS THREAD SAFE**********/
unsigned int insert_nary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int wId){
  pthread_rwlock_wrlock(&insert_nary_clause_lock);
  //we will have problems here for keeping the same search for each thread
  dassert(cl->size > 3);
  int i,j;
  int index=-1;
  NClause nclause;
  vec_literal_sort(cl->lits.a,cl->size);

  /*Check if alreadyAdded*/
  int listSize;
  ts_vec_size(listSize, cdb->nDB);
  dassert( cdb->numNClauses == listSize );

  for( i=0; i<cdb->numNClauses; i++ ){
    ts_vec_ith( nclause, cdb->nDB, i );

    if(kv_size(nclause.lits) == cl->size){
      index=i;
      for( j=0; j<cl->size; j++ ){
	if( kv_A(nclause.lits,j) != kv_A(cl->lits,j) ){
	  index = -1;
	  break;
	}
      }
    }

    if(index>0) break; //if clause found, do not look anymore
  }
  /*********************/

  /*In case the clause already exists, modify flags if it's learned*/
  if(index!=-1){
    if(!isOriginal){
      ts_vec_ith( nclause, cdb->nDB, index );
      nclause.flags[wId] = true;
      ts_vec_set_ith(NClause,cdb->nDB,index,nclause);
    }
    return index; //this is returned for watches in threads
  }
  /******************************************************************/

  /***********In case it doesn't exist, insert it***/
  //Init flags

  if(isOriginal){
    for( i=0; i<cdb->numWorkers; i++ ){
      nclause.flags[i] = true;
    }
  }else{
    for( i=0; i<cdb->numWorkers; i++ ){
      if(i==wId) nclause.flags[i] = true;
      else nclause.flags[i] = false;
    }
  }
  //Insert literals
  for(i=0;i<cl->size;i++){
    kv_A( nclause.lits, i ) = kv_A( cl->lits, i );
  }
  nclause.is_original = isOriginal;

  //update clauseDB
  ts_vec_push_back(NClause, cdb->nDB, nclause);

  if(isOriginal) cdb->numInputClauses++;
  cdb->numNClauses++;
  cdb->numClauses++;
  //  dassert(ts_vec_size(NClause, cdb->nDB)==cdb->numNClauses);
  return cdb->numNClauses-1;
  /**************************************************/
  pthread_rwlock_wrlock(&insert_nary_clause_lock);
}

#endif /* _CLAUSEDB_C_ */
