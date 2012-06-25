#ifndef _CLAUSEDB_C_
#define _CLAUSEDB_C_

#include "clausedb.h"

Clause inputClause;
unsigned int nInsertedClauses;
pthread_rwlock_t insert_unitary_clause_lock = PTHREAD_RWLOCK_INITIALIZER;
//pthread_rwlock_t insert_binary_clause_lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t insert_ternary_clause_lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t insert_nary_clause_lock = PTHREAD_RWLOCK_INITIALIZER;
pthread_rwlock_t *insert_binary_clause_locks;
pthread_rwlock_t donelock = PTHREAD_RWLOCK_INITIALIZER;

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
  cdb->solved          = false;
  cdb->numVars         = nVars;
  cdb->numWorkers      = nWorkers;

  cdb->numClauses           = 0;
  cdb->numUnits             = 0;
  cdb->numOriginalUnits     = 0;
  cdb->numBinaries          = 0;
  cdb->numOriginalBinaries  = 0;
  cdb->numNClauses          = 0;
  cdb->numOriginalNClauses  = 0;
  cdb->numInputClauses      = 0;

  /*Init Unitary clauses database*/
  //  printf("Init unit db\n");
  kv_init(cdb->uDB); //init vector with 0 elements
  kv_resize(Literal, cdb->uDB, nVars); //We now max nvars can be true
  kv_size(cdb->uDB)=0;
  /******************************/

  /*Init Binary clauses database*/
  //  printf("Init bin db\n");
  cdb->bDB = (BinList*)malloc((2*nVars+2)*sizeof(BinList));
  //cdb->bListsSize = (unsigned int*)malloc((2*nVars+2)*sizeof(unsigned int));

  /*Init each of the binary clause database elements*/
  for(int i=0;i<2*(nVars+1);i++){
    cdb->bDB[i].size=0;
    cdb->bDB[i].posInLastNode=0;
    cdb->bDB[i].firstNode=(BinNode*)malloc(sizeof(BinNode));
    for(int j=0;j<(CACHE_LINE_SIZE/4)-1;j++){
      cdb->bDB[i].firstNode->litList[j]=0;
    }
    cdb->bDB[i].firstNode->nextNode=NULL;
    cdb->bDB[i].lastNode = cdb->bDB[i].firstNode;
  }
  /******************************/



  /*Init binary locks*/
  insert_binary_clause_locks=(pthread_rwlock_t*)malloc(2*(nVars+1)*sizeof(pthread_rwlock_t));

  for(int i=0; i<2*nVars+2;i++){
    pthread_rwlock_init(&(insert_binary_clause_locks[i]), NULL);
    //insert_binary_clause_locks[i] = PTHREAD_RWLOCK_INITIALIZER;
  }
  /******************/

  /*******************************/
  //  printf("Init n db\n");
  kv_init(cdb->nDB); //init vector with 0 elements
  kv_resize(NClause, cdb->nDB, MAX_NARY_CLAUSES); //reserve memory so that references don't change
  /*******************************/

  /*Init random numbers vector*/
  srandom(0);
  //  printf("Init Random numbers\n");
  for(int i=0;i<MAX_RANDOM_NUMBERS;i++){
    cdb->randomNumbers[i] = random();
  }
  /****************************/

  cdb->clIndex = (kvec_t(NClause)*)malloc((2*nVars+2)*sizeof(kvec_t(NClause)));
  cdb->indexInputClauses = (unsigned int*)malloc((2*nVars+2)*sizeof(unsigned int));
  for(int i=0;i<2*nVars+2;i++){
    cdb->indexInputClauses[i]=0;
  }
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
      insert_binary_clause(cdb, &inputClause, true, 0);
      //printf("Binary Clause read\n");
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
  int listSize = kv_size(cdb->uDB);
  //  ts_vec_size(listSize, cdb->uDB);
  dassert(listSize == cdb->numUnits);

  /*************Hack for same search*************/
  //We assume each thread learns the next ternary in the same order
  if(!isOriginal){
    for(int i=lastThUnit;i<listSize;i++){
      if( kv_A(cdb->uDB,i) == kv_A(cl->lits,0)){
	alreadyInList = true;
	break;
	/*FOR DEBUG*/
	/* ts_vec_ith(unitInList,cdb->uDB,lastThUnit); */
	/* dassert(unitInList == kv_A(cl->lits,0)); */
	/***********/
      }
    }
  }
  /*********************************************/

  /*If it isn't already in the uDB, add it*/
  if(!alreadyInList){
    kv_push( Literal, cdb->uDB, kv_A(cl->lits,0) ); //Here vector can be rellocated
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
void insert_binary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, BinNode* thLastL1){
  //  printf("Before binary lock\n");
  //  pthread_rwlock_wrlock(&insert_binary_clause_lock);
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

  pthread_rwlock_wrlock(&(insert_binary_clause_locks[not_l1]));


  /*************Search for already learned bins*************/
  if(!isOriginal){
    //printf("A none original bin learned\n");
    BinNode* currentNode = thLastL1;
    unsigned int i=0;
    while(currentNode!=NULL && (currentNode!=cdb->bDB[not_l1].lastNode || i!= cdb->bDB[not_l1].posInLastNode) && !alreadyInList ){
      if(currentNode->litList[i] == l2 ){
	//printf("found in list\n");
	alreadyInList = true;
	break;
      }

      if(i==(CACHE_LINE_SIZE/4)-2){
	currentNode = currentNode->nextNode;
	i=0;
      }else{
	i++;
      }
    }
  }
  /*********************************************/

  /*Insert and update, if not previously inserted*/
  if(!alreadyInList){
    BinNode* newNode;
    if(cdb->bDB[not_l1].posInLastNode==(CACHE_LINE_SIZE/4)-1){
      newNode=(BinNode*)malloc(sizeof(BinNode));
      for(int j=0;j<(CACHE_LINE_SIZE/4)-1;j++){
	newNode->litList[j]=0;
      }
      newNode->nextNode=NULL;
      cdb->bDB[not_l1].lastNode->nextNode=newNode;
      cdb->bDB[not_l1].lastNode=newNode;
      cdb->bDB[not_l1].posInLastNode=0;
    }
    
    if(cdb->bDB[not_l2].posInLastNode==(CACHE_LINE_SIZE/4)-1){
      newNode=(BinNode*)malloc(sizeof(BinNode));
      for(int j=0;j<(CACHE_LINE_SIZE/4)-1;j++){
	newNode->litList[j]=0;
      }
      newNode->nextNode=NULL;
      cdb->bDB[not_l2].lastNode->nextNode=newNode;
      cdb->bDB[not_l2].lastNode=newNode;
      cdb->bDB[not_l2].posInLastNode=0;
    }
    
    //    cdb->bDB[not_l1][cdb->bListsSize[not_l1]++] = l2;
    //    cdb->bDB[not_l2][cdb->bListsSize[not_l2]++] = l1;
    cdb->bDB[not_l1].lastNode->litList[cdb->bDB[not_l1].posInLastNode++] = l2;
    cdb->bDB[not_l2].lastNode->litList[cdb->bDB[not_l2].posInLastNode++] = l1;
    cdb->bDB[not_l1].size++;
    cdb->bDB[not_l2].size++;
    
    //update clauseDB stats
    if(isOriginal){
      cdb->numOriginalBinaries++;
      cdb->numInputClauses++;
    }
    
    cdb->numBinaries++;
    cdb->numClauses++;
  }
  /********************************************/
  pthread_rwlock_unlock(&(insert_binary_clause_locks[not_l1]));
}

/****MAKE THIS THREAD SAFE**********/
void insert_nary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int wId, NClause** ptrToNClause, unsigned int lastThNary){
  //  printf("Before nary lock\n");
  pthread_rwlock_wrlock(&insert_nary_clause_lock);
  //  printf("After nary lock\n");
  //we will have problems here for keeping the same search for each thread
  dassert(cl->size >= 3);
  int i;
  bool alreadyInList = false;

  //  vec_literal_sort(cl->lits.a,cl->size);
  vec_literal_sort(cl,cl->size);
  int listSize;
  listSize =kv_size(cdb->nDB);

  //printf("wId %d trying to insert nclause\n",wId);

  //printf("Last nary is %d\n",lastThNary);
  //printf("Num Nary clauses is %d\n",cdb->numNClauses);
  //printf("listSize is %d\n",listSize);
  //We assume each thread learns the next nary in the same order
  unsigned int smallestLit = lit_as_uint(kv_A(cl->lits,0));
  /* if(!isOriginal){ */
  /*   int j; */
  /*   unsigned int posInNDB; */
  /*   for(i=cdb->indexInputClauses[smallestLit];i<kv_size(cdb->clIndex[smallestLit]);i++){ */
  /*     posInNDB = kv_A(cdb->clIndex[smallestLit],i); */
  /*     if(kv_A(cdb->nDB,posInNDB).lits[0]==cl->size){ */
  /* 	for(j=0;j<cl->size;j++){ */
  /* 	  if( kv_A(cdb->nDB,posInNDB).lits[j+1]!=kv_A(cl->lits,j)) break; */
  /* 	} */
  /* 	if(j==cl->size){ */
  /* 	  alreadyInList = true; */
  /* 	  *ptrToNClause = &kv_A(cdb->nDB,posInNDB); */
  /* 	  (*ptrToNClause)->flags[wId]=true; */
  /* 	  break; */
  /* 	} */
  /*     } */
  /*   } */
  /* } */
  //for(int i=cdb->numOriginalNClauses;i<listSize;i++){
  //if( kv_A(cdb->nDB,i).lits[0] == cl->size){
  //for(j=0;j<cl->size;j++){
  //if(kv_A(cdb->nDB,i).lits[j+1]!=kv_A(cl->lits,j)) break;
  //}
  //if(j==cl->size){
  // alreadyInList = true;
  //*ptrToNClause = &kv_A(cdb->nDB,lastThNary);
  //(*ptrToNClause)->flags[wId]=true;
  //break;
  //}
  //}
  //}
  //}
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
    nclause.lits = (Literal*)malloc((cl->size+1)*sizeof(Literal));
    //nclause.size = cl->size;
    //kv_resize(Literal, nclause.lits, cl->size);
    //kv_size(nclause.lits) = cl->size;
    nclause.lits[0] = cl->size;
    for(i=0;i<cl->size;i++){
      nclause.lits[i+1] = kv_A( cl->lits, i );
    }
    nclause.is_original = isOriginal;
    //  printf("nclause literals added\n");
    
    //update clauseDB
    //kv_push(unsigned int, cdb->clIndex[smallestLit], kv_size(cdb->nDB)); //new clause will be in kv_size position in nDB.
    kv_push(NClause, cdb->nDB, nclause);
    *ptrToNClause = &kv_A(cdb->nDB, listSize);
    
    //    printf("Pointer to NClause is %d\n", *ptrToNClause);
    if(isOriginal){
      cdb->indexInputClauses[smallestLit]++;
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

void setSolved(ClauseDB* cdb){
  pthread_rwlock_wrlock(&donelock);
  cdb->solved = true;
  pthread_rwlock_unlock(&donelock);
}

bool isSolved(ClauseDB* cdb){
  bool wasSolved=false;
  pthread_rwlock_wrlock(&donelock);
  wasSolved = cdb->solved;
  pthread_rwlock_unlock(&donelock);
  return wasSolved;
}
#endif /* _CLAUSEDB_C_ */
