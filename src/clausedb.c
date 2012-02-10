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

  //  printf("Init input clause\n");
  kv_init(inputClause.lits); //init inputClause
  kv_resize(Literal, inputClause.lits, nVars);//Set size of nVars
  kv_size(inputClause.lits) = nVars;

  inputClause.size = 0; //the vector is nVars size, so we keep the number of lits actually added in other counter.
  nInsertedClauses=0;


  //printf("Nvars is %d \n",nVars);
  //exit(0);
  
  ClauseDB* cdb = (ClauseDB*)malloc(sizeof(ClauseDB));

  //  printf("Init parameters\n");
  cdb->numVars         = nVars;
  cdb->numWorkers      = nWorkers;

  cdb->numClauses           = 0;
  cdb->numUnits             = 0;
  cdb->numOriginalUnits     = 0;
  cdb->numBinaries          = 0;
  cdb->numOriginalBinaries  = 0;
  cdb->numTernaries         = 0;
  cdb->numOriginalTernaries = 0;
  cdb->numNClauses          = 0;
  cdb->numOriginalNClauses  = 0;
  cdb->numInputClauses      = 0;

  /* ts_vec_init(cdb->numOriginalBinaries); */
  /* ts_vec_resize(unsigned int, cdb->numOriginalBinaries, 2*(nVars+1)); */
  /* for(int i=0;i<2*(nVars+1);i++){ */
  /*   ts_vec_set_ith(unsigned int, cdb->numOriginalBinaries, i, 0); */
  /* } */

  /*Init Unitary clauses database*/
  //  printf("Init unit db\n");
  ts_vec_init(cdb->uDB); //init vector with 0 elements
  /******************************/

  /*Init Binary clauses database*/
  //  printf("Init bin db\n");
  kv_init(cdb->bDB);//init vector 
  kv_resize(ts_vec_t(Literal), cdb->bDB, 2*(nVars+1) ); //We know size is fixed to 2*(nVars+1) elements
  kv_size(cdb->bDB)= 2*(nVars+1);

  /*Init each of the binary clause database elements*/
  for(int i=0;i<2*(nVars+1);i++){
    ts_vec_init(kv_A(cdb->bDB,i));
    ts_vec_resize(Literal, kv_A(cdb->bDB,i), 100 );
  }
  /******************************/

  /*Init Ternary clauses database*/
  //  printf("Init ternary db\n");
  ts_vec_init(cdb->tDB); //init vector with 0 element
  ts_vec_resize(TClause, cdb->tDB, MAX_TERNARY_CLAUSES); //reserve memory so that references don't change
  /*******************************/

  /*******************************/
  //  printf("Init n db\n");
  ts_vec_init(cdb->nDB); //init vector with 0 elements
  ts_vec_resize(NClause, cdb->nDB, MAX_NARY_CLAUSES); //reserve memory so that references don't change
  /*******************************/

  /*Init 3watches structures*/
  kv_init( cdb->ternaryWatches );
  //  printf("Init 3 watches structure\n");
  kv_resize(ts_vec_t(TClause*), cdb->ternaryWatches,  2*(nVars+1) );
  kv_size(cdb->ternaryWatches) = 2*(nVars+1);
  //  printf("vector resized\n");
  for(int i=0;i<2*(nVars+1);i++){
    ts_vec_init( kv_A(cdb->ternaryWatches,i) );
    ts_vec_resize(Literal, kv_A(cdb->ternaryWatches,i), 100 );
  }
  /*************************/


  /*Init random numbers vector*/
  srandom(0);
  //  printf("Init Random numbers\n");
  for(int i=0;i<MAX_RANDOM_NUMBERS;i++){
    cdb->randomNumbers[i] = random();
  }
  /****************************/

  return cdb;
}

/*Insertion sort for normal clauses, given a size*/

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
}

/*Adding an input literal (from file) to the input clause*/
unsigned int add_input_literal(ClauseDB* cdb, Literal l){
  /*checking if the clause is complete.
    A literal with value 0 indicates this.*/
  if(l==0){
    switch(inputClause.size){
    case 0:
      return 20; //empty clause in original formula
    case 1:
      insert_unitary_clause(cdb, &inputClause, true, 0);
      //printf("Unit Clause read\n");
      break;
    case 2:
      //printf("About to add bin Clause\n");
      insert_binary_clause(cdb, &inputClause, true, 0, 0);
      //printf("Binary Clause read\n");
      break;
    case 3:
      //printf("About to add Ternary Clause\n");
      ;TClause* ptrToTernary = NULL;
      insert_ternary_clause(cdb, &inputClause,true, 0, &ptrToTernary, 0);
      //printf("Ternary Clause read\n");
      break;
    default:
      ;NClause* ptrToNary = NULL;
      //printf("About to add N Clause\n");
      insert_nary_clause(cdb, &inputClause, true, 0, &ptrToNary, 0);
      //printf("N Clause read\n");
    }    
    //new clause will be read
    //printf("Clause read\n");
    inputClause.size = 0;
  }else{
    /*if the clause is not complete, we add the literal to it*/
    kv_A(inputClause.lits,inputClause.size) = l;
    inputClause.size++;
  }
  return 0;
}

/*Inserting, in the clause database, an input clause that only has one literal*/
void insert_unitary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int lastThUnit){
  //  printf("Before unit lock\n");
  pthread_rwlock_wrlock(&insert_unitary_clause_lock);
  //  printf("After unit lock\n");
  dassert(cl->size == 1);
  bool alreadyInList=false;
  int listSize;
  ts_vec_size(listSize, cdb->uDB);
  dassert(listSize == cdb->numUnits);

  /*************Hack for same search*************/
  //We assume each thread learns the next ternary in the same order
  if(!isOriginal){
    if(listSize > lastThUnit){
      Literal unitInList;
      alreadyInList = true;
      /*FOR DEBUG*/
      /* ts_vec_ith(unitInList,cdb->uDB,lastThUnit); */
      /* dassert(unitInList == kv_A(cl->lits,0)); */
      /***********/
    }
  }
  /*********************************************/


  /*If it isn't already in the uDB, add it*/
  if(!alreadyInList){
    ts_vec_push_back( Literal, cdb->uDB, kv_A(cl->lits,0) );
    if(isOriginal) {
      cdb->numInputClauses++;
      cdb->numOriginalUnits++;
    }
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
void insert_binary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int thLast1, unsigned int thLast2){
  //  printf("Before binary lock\n");
  pthread_rwlock_wrlock(&insert_binary_clause_lock);
  //  printf("After binary lock\n");
  //we will have problems here for keeping the same search for each thread
  dassert(cl->size == 2);
  /*We are storing implications, so we want to know the negation of each literal
   that belongs to the binary clause*/
  Literal l1,l2;
  l1 = kv_A(cl->lits,0);
  l2 = kv_A(cl->lits,1);

  unsigned int not_l1,not_l2;
  not_l1 = lit_as_uint(-l1);
  not_l2 = lit_as_uint(-l2);

  bool alreadyInList = false;

  /*************Hack for same search*************/
  //We assume each thread learns the next binary in the same order
  if(!isOriginal){
    int listSize;
    Literal otherLit;
    ts_vec_size(listSize, kv_A(cdb->bDB, not_l1));
    if(listSize > thLast1){
      alreadyInList = true;
      /*FOR DEBUG*/
      /* ts_vec_ith(otherLit, kv_A(cdb->bDB, not_l1), thLast1); */
      /* dassert(otherLit == l2); */
      /* ts_vec_ith(otherLit, kv_A(cdb->bDB, not_l2), thLast2); */
      /* dassert(otherLit == l1); */
      /**********/
    }
  }
  /*********************************************/

  /*Insert and update, if not previously inserted*/
  if(!alreadyInList){
    unsigned int lastLit1Lst, lastLit2Lst;
    ts_vec_push_back(Literal,kv_A(cdb->bDB, not_l1),l2);
    ts_vec_size(lastLit1Lst,kv_A(cdb->bDB, not_l1));
    lastLit1Lst--;
    //If it is not in list, it's not in the other list either
    ts_vec_push_back(Literal, kv_A(cdb->bDB, not_l2),l1);
    ts_vec_size(lastLit2Lst,kv_A(cdb->bDB, not_l2));
    lastLit2Lst--;
    //update clauseDB stats
    if(isOriginal){
      /* unsigned int nOrLit1, nOrLit2; */
      /* ts_vec_ith(nOrLit1, cdb->numBinaries, not_l1); */
      /* ts_vec_ith(nOrLit2, cdb->numBinaries, not_l2); */
      /* nOrLit1++; */
      /* nOrLit2++; */
      /* ts_vec_set_ith(nOrLit1, cdb->numBinaries, not_l1); */
      /* ts_vec_set_ith(nOrLit2, cdb->numBinaries, not_l2); */
      cdb->numOriginalBinaries++;
      cdb->numInputClauses++;
    }

    cdb->numBinaries++;
    cdb->numClauses++;
  }
  /********************************************/
  pthread_rwlock_unlock(&insert_binary_clause_lock);
}

/*Function for sorting literals in a vector.
 Sorts from lower to highest*/

/* void vec_literal_sort(Literal *lits, unsigned int size) { */

/*     struct sort_node *head; */
/*     struct sort_node *current; */
/*     struct sort_node *next; */
/*     int i = 0; */

/*     kvec_t(Literal) tmp; */
/*     tmp.a = lits; */

/*     head = NULL; */
/*     /\* insert the numbers into the linked list *\/ */
/*     for (i = 0; i < size; i++) */
/*         head = addsort_node(kv_A(tmp, i), head); */

/*     /\* sort the list *\/ */
/*     head = mergesort(head); */

/*     /\*Store the list back into the vector*\/ */
/*     i = 0; */
/*     for (current = head; current != NULL; current = current->next) */
/*         kv_A(tmp, i++) = current->number; */

/*     /\* free the list *\/ */
/*     for (current = head; current != NULL; current = next) */
/*         next = current->next, free(current); */
/* } */


/************MAKE THIS THREAD SAFE**********/
/* To insert a clause with 3 literals into the tDB*/
void insert_ternary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, int wId, TClause** ptrToTernary, unsigned int lastThTernary) {
  //  printf("Before ternary lock\n");
  pthread_rwlock_wrlock(&insert_ternary_clause_lock);
  //  printf("After ternary lock\n");
  dassert(cl->size == 3);
  int i;
  vec_literal_sort(cl, cl->size);

  bool alreadyInList = false;
  int listSize;
  ts_vec_size(listSize, cdb->tDB);

  //Init flags
  /*************Hack for same search*************/
  //We assume each thread learns the next ternary in the same order
  if(!isOriginal){
    if(listSize > lastThTernary){
      alreadyInList = true;
      ts_vec_ith_ma(*ptrToTernary,cdb->tDB,lastThTernary);
      dassert((*ptrToTernary)->flags[wId]==false);
      (*ptrToTernary)->flags[wId]=true; 
      /*FOR DEBUG*/
      /* for (i = 0; i < 3; i++) { */
      /* 	dassert((*ptrToTernary)->lits[i] == kv_A(cl->lits, i)); */
      /* } */
      /***********/
    }
  }
  /*********************************************/

  if(!alreadyInList){
    TClause ternary;
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

    /*add this clause position in tDB to each literal watches*/
    unsigned int litIndex;

    /*Return a pointer to the actual ternary inserted clause*/
    ts_vec_ith_ma(*ptrToTernary,cdb->tDB,listSize);
    
    for (i = 0; i < 3; i++) {
      litIndex = lit_as_uint(ternary.lits[i]);
      ts_vec_push_back(TClause*, kv_A(cdb->ternaryWatches, litIndex), *ptrToTernary);
    }
    if (isOriginal){
      cdb->numInputClauses++;
      cdb->numOriginalTernaries++;
    }
    cdb->numTernaries++;
    cdb->numClauses++;
    dassert( listSize+1 == cdb->numTernaries );
    /************************************************/
  }
  pthread_rwlock_unlock(&insert_ternary_clause_lock);
}

/****MAKE THIS THREAD SAFE**********/
void insert_nary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int wId, NClause** ptrToNClause, unsigned int lastThNary){
  //  printf("Before nary lock\n");
  pthread_rwlock_wrlock(&insert_nary_clause_lock);
  //  printf("After nary lock\n");
  //we will have problems here for keeping the same search for each thread
  dassert(cl->size > 3);
  int i;
  bool alreadyInList = false;

  //  vec_literal_sort(cl->lits.a,cl->size);
  vec_literal_sort(cl,cl->size);
  int listSize;
  ts_vec_size(listSize, cdb->nDB);

  /*************Hack for same search*************/
  //We assume each thread learns the next ternary in the same order
  if(!isOriginal){
    if(listSize > lastThNary){
      alreadyInList = true;
      ts_vec_ith_ma(*ptrToNClause,cdb->nDB,lastThNary);
      dassert((*ptrToNClause)->flags[wId]==false);

      (*ptrToNClause)->flags[wId]=true;
      dassert( kv_size((*ptrToNClause)->lits) == cl->size );
      /*FOR DEBUG*/
      /* for (i = 0; i < cl->size; i++) { */
      /* 	dassert(kv_A((*ptrToNClause)->lits,i) == kv_A(cl->lits, i)); */
      /* } */
      /***********/
    }
  }
  /*********************************************/

  if(!alreadyInList){
    NClause nclause;
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
    kv_init(nclause.lits);
    kv_resize(Literal, nclause.lits, cl->size);
    kv_size(nclause.lits) = cl->size;
    for(i=0;i<cl->size;i++){
      kv_A( nclause.lits, i ) = kv_A( cl->lits, i );
    }
    nclause.is_original = isOriginal;
    //  printf("nclause literals added\n");
    
    //update clauseDB
    ts_vec_push_back(NClause, cdb->nDB, nclause);
    ts_vec_ith_ma(*ptrToNClause, cdb->nDB, listSize);
    
    //    printf("Pointer to NClause is %d\n", *ptrToNClause);
    if(isOriginal){
      cdb->numInputClauses++;
      cdb->numOriginalNClauses++;
    }

    cdb->numNClauses++;
    cdb->numClauses++;
    dassert( listSize+1 == cdb->numNClauses );
    /**************************************************/
  }
   pthread_rwlock_unlock(&insert_nary_clause_lock);
}

void clause_database_resize_vectors(ClauseDB* cdb){

  unsigned int currentSize;
  /*Init each of the binary clause database elements*/
  for(int i=0;i<2*(cdb->numVars+1);i++){
    ts_vec_max(currentSize,kv_A(cdb->bDB,i));
    ts_vec_resize(Literal, kv_A(cdb->bDB,i), 4*currentSize );
  }
  /******************************/

  //  printf("vector resized\n");
  for(int i=0;i<2*(cdb->numVars+1);i++){
    ts_vec_max(currentSize,kv_A(cdb->ternaryWatches,i));
    ts_vec_resize(TClause*, kv_A(cdb->ternaryWatches,i), 4*currentSize );
  }
  /*************************/

}

#endif /* _CLAUSEDB_C_ */
