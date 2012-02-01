#ifndef _CLAUSEDB_C_
#define _CLAUSEDB_C_

#include "clausedb.h"

Clause inputClause;
unsigned int nInsertedClauses;

ClauseDB* init_clause_database(unsigned int nVars, unsigned int nWorkers){

  kv_init(inputClause.lits); //init inputClause
  kv_resize(Literal, inputClaue.lits, nVars);//Set size of nVars

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
  kvec_init(cdb->bDB,2*(nVars+1));//init vector 
  kvec_resize( ts_vec(Literal), cdb->bDB, 2*(nVars+1) ); //We know size is fixed to 2*(nVars+1) elements

  /*Init each of this elements*/
  for(i=0;i<=2*(nVars+1);i++){
    ts_vec_init( kvec_A(cdb->bDB,i) );
  }
  /******************************/

  /*Init Ternary clauses database*/
  ts_vec_init(cdb->tDB); //init vector with 0 elements
  /*******************************/

  /*******************************/
  ts_vec_init(cdb->nDB); //init vector with 0 elements
  /*******************************/

  /*Init 3watches structures*/
  kvec_init( cdb->ThreeWatches );
  kvec_resize( kvec(unsigned int), cdb->ThreeWatches,  2*(nVars+1) );
  for(i=0;i<=2*(nVars+1);i++){
    kvec_init( kvec_A(cdb->ThreeWatches,i) );
  }
  /*************************/


  /*Init random numbers vector*/
  srandom(0);
  kvec_init( randomNumbers );
  kvec_resize( int, randomNumbers, 10000000 );
  for(i=0;i<10000000;i++){
    kvec_A(randomNumbers,i) = random();
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
      if( kvec_A(cl->lits,i) > kvec_A(cl->lits,j)){
	aux = kvec_A(cl->lits,i);
	kvec_A(cl->lits,i) = kvec_A(cl->lits,j);
	kvec_A(cl->lits,j) = aux;
      }
    }
  }
}

unsigned int add_input_literal(ClauseDB* cdb, Literal l){
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
    cdb->numInputClauses++;
    nInsertedClauses++;
  }else{
    kvec.A(inputClause.lits,inputClause.size) = l;
    inputClause.size++;
  }
}

void insert_unitary_clause(ClauseCB* cdb, Clause *cl){
  dassert(cl->size == 1);
  int i;
  bool alreadyAdded=false;
  Literal unitInList;
  int listSize;
  ts_vec_size(listSize, cdb->uDB);
  dassert(listSize == cl->numUnits);

  for(i=0; i < cl->numUnits; i++){
    ts_vec_ith(unitInList, cdb->uDB, i);
    if( kvec_A(cl->lits,0) == unitInList ) {
      alreadyAdded=true;
      break;
    }
  }
  
  if(!alreadyAdded){
    ts_vec_push_back( cdb->uDB, vec_ith(cl->lits,0) );
    if(isOriginal) cdb->numInputClauses++;
    cdb->numUnits++;
    cdb->numClauses++;
  }
}


/*******************Make this Thread safe****************************/
void insert_binary_clause(ClauseCB* cdb, Clause *cl, bool isOriginal){
  //we will have problems here for keeping the same search for each thread
  int i;

  dassert(cl->size == 2);

  Literal l1,l2;
  l1 = vec_ith(cl->lits,0);
  l2 = vec_ith(cl->lits,1);
  unsigned int not_l1,not_l2;
  not_l1 = lit_as_uint(-l1);
  not_l2 = lit_as_uint(-l2);
  
  ts_vec(Literal) list:

  list = kvec_A(cdb->bDB, not_l1);
  /*Search to see if element is already in list*/
  bool alreadyInList=false;
  Literal lInList;  
  int listSize;
  ts_vec_size(listSize, list);
  for(i=0;i<listSize;i++){
    ts_vec_ith(lInList,list,i);
    if(lInList == l2){
      alreadyInList=true
    }
  }
  /*********************************************/

  /*Insert and update, if not previously inserted*/
  if(!alreadyInList){
    ts_vec_push_back(Literal,list,l2);
    //If it is not in list, it's not in the other list either
    list = kvec_A(cdb->bDB, not_l2);
    ts_vec_push_back(Literal, list,l1);
    //update clauseDB stats
    if(isOriginal) cdb->numInputClauses++;
    cdb->numBinaries++;
    cdb->numClauses++;
  }
  /********************************************/

}

/************MAKE THIS THREAD SAFE**********/
void insert_ternary_clause(ClauseCB* cdb, Clause *cl, bool isOriginal, int wId)
{
  dassert(cl->size == 3);
  int i,j;
  int index=-1;
  TClause ternary;

  vec_literal_sort(cl->lits,cl->size); //Implement

  /*Check if alreadyAdded*/
  int listSize;
  ts_vec_size(listSize, cdb->tDB);
  dassert(cdb->numTernaries == listSize);
  for( i=0; i<cdb->numTernaries; i++){
    ts_vec_ith( ternary, cdb->tDB, i );
    index=i;
    for( j=0; j<3; j++){
      if( ternary.lits[j] != kvec_A(cl->lits, j) ){
	index = -1;
	break;
      }
    }
    if(index>0) break;
  }
  /*********************/

  /*In case the clause already exists, modify flags if it's learned*/
  if(index!=-1){
    if(!isOriginal){
      ts_vec_ith( ternary, cdb->tDB, index);
      ts_vec_set_ith(bool, ternary.flags, wId, true );
    }
    return;
  }
  /********************************************************/

  /*In case it doesn't exist, insert it and watch*/
  //Init flags
  ts_vec_init(ternary.flags);
  ts_vec_resize( bool, ternary.flags, cdb->numWorkers );

  if(isOriginal){
    for(i=0;i<cdb->numWorkers;i++){
      ts_vec_set_ith(bool, ternary.flags, i, true);
    }
  }else{
    for(i=0;i<cdb->numWorkers;i++){
      if(i==wId)
	ts_vec_set_ith(bool, ternary.flags, i, true);
      else
	ts_vec_set_ith(bool, ternary.flags, i, false);
    }
  }

  //Insert literals
  for(i=0;i<3;i++){
    ternary.lits[i] = vec_A(cl->lits,i);
  }
  //update clauseDB
  ts_vec_push_back(TClause, cdb->tDB, ternary);

  /*add this clause position in tDB to each neg literal watches*/
  unsigned int litIndex;
  unsigned int indexOfNewClause;
  ts_vec_size(indexOfNewClause, cdb->tDB);
  indexOfNewClause--;

  for( i=0; i<3; i++){
    litIndex = lit_as_uint(-ternary.lits[i]);
    vec_push_back( vec_ith(cdb->ThreeWatches, litIndex), indexOfNewClause );
  }

  if(isOriginal) cdb->numInputClauses++;
  cdb->numTernaries++;
  cdb->numClauses++;
  //  dassert(ts_vec_size(TClause, cdb->tDB)==cdb->numTernaries);
  /************************************************/
}


/****MAKE THIS THREAD SAFE**********/
unsigned int insert_nary_clause(ClauseCB* cdb, Clause *cl, bool isOriginal, unsigned int wId){
  //we will have problems here for keeping the same search for each thread
  dassert(cl->size > 3);
  int i,j;
  int index=-1;
  NClause nclause;
  vec_literal_sort(cl->lits,cl->size); //To implement

  /*Check if alreadyAdded*/
  int listSize;
  ts_vec_size(listSize, cdb->nDB);
  dassert( cdb->numNClauses == listSize );

  for( i=0; i<cdb->numNClauses; i++ ){
    ts_vec_ith( nclause, cdb->tDB, i );

    if(kvec_size(nclause.lits) == cl->size){
      index=i;
      for( j=0; j<cl->size; j++ ){
	if( kvec_A(nclause.lits,j) != kvec_A(cl->lits,j) ){
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
      ts_vec_ith( nclause, cdb->tDB, index );
      ts_vec_set_ith( bool, nclause.flags, wId, true);
    }
    return index; //this is returned for watches in threads
  }
  /******************************************************************/

  /***********In case it doesn't exist, insert it***/
  //Init flags
  ts_vec_init(nclause.flags);
  ts_vec_resize( bool, nclause.flags, cdb->numWorkers );

  if(isOriginal){
    for( i=0; i<cdb->numWorkers; i++ ){
      ts_vec_set_ith( bool, nclause.flags, i, true );
    }
  }else{
    for( i=0; i<cdb->numWorkers; i++ ){
      if(i==wId) ts_vec_set_ith( bool, nclause.flags, i, true );
      else ts_vec_set_ith( bool, nclause.flags, i, false );
    }
  }
  //Insert literals
  for(i=0;i<cl->size;i++){
    kvec_A( nclause.lits, i ) = kvec_A( cl->lits, i );
  }
  nclause.isOriginal = isOriginal;

  //update clauseDB
  ts_vec_push_back(NClause, cdb->nDB, nclause);

  if(isOriginal) cdb->numInputClauses++;
  cdb->numNClauses++;
  cdb->numClauses++;
  //  dassert(ts_vec_size(NClause, cdb->nDB)==cdb->numNClauses);
  return cdb->numNClauses-1;
  /**************************************************/
}

#endif /* _CLAUSEDB_C_ */
