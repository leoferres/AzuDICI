#ifndef _CLAUSEDB_C_
#define _CLAUSEDB_C_

#include "clausedb.h"

Clause inputClause;
unsigned int nInsertedClauses;

ClauseDB* init_clause_database(unsigned int nVars, unsigned int nWorkers){

  vec_init(inputClause.lits,nVars); //init inputClause
  inputClause.size = 0;
  nInsertedClauses=0;

  ClauseDB* cdb;

  cdb->numVars         = nVars;
  cdb->numWorkers      = nWorkers;

  cdb->numClauses      = 0;
  cdb->numUnits        = 0;
  cdb->numBinaries     = 0;
  cdb->numTernaries    = 0;
  cdb->numNClauses     = 0;
  cdb->numInputClauses = 0;

  /*Init Unitary clauses database*/
  ts_vec_init(cdb->uDB,0); //init vector with 0 elements
  /******************************/

  /*Init Binary clauses database*/
  vec_init(cdb->bDB,2*(nVars+1));//init vector with 2*(nVars+1) elements
  /*Init each of this elements*/
  for(i=0;i<=2*(nVars+1);i++){
    ts_vec_init( vec_ith(cdb->bDB,i), 0 );
  }
  /******************************/

  /*Init Ternary clauses database*/
  ts_vec_init(cdb->tDB,0); //init vector with 0 elements
  /*******************************/

  /*******************************/
  ts_vec_init(cdb->nDB,0); //init vector with 0 elements
  /*******************************/

  /*Init 3watches structures*/
  vec_init( cdb->3Watches, 2*(nVars+1) );
  for(i=0;i<=2*(nVars+1);i++){
    vec_init( vec_ith(cdb->3Watches,i), 0 );
  }
  /*************************/
}

unsigned int add_input_literal(ClauseDB* cdb, Literal l){
  if(l==0){
    switch(inputClause.size){
    case 0:
      return 20; //empty clause in original formula
    case 1:
      insert_unitary_clause(cdb, &inputClause);
      break;
    case 2:
      insert_binary_clause(cdb, &inputClause);
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
    inputClause.lits[inputClause.size]=l;
    inputClause.size++;
  }
}

void insert_unitary_clause(ClauseCB* cdb, Clause *cl){
  dassert(cl->size == 1);
  int i;
  bool alreadyAdded=false;
  for(i=0; i < ts_vec_size(cdb->uDB); i++){
    if( vec_ith(cl->lits,0) == ts_vec_ith(cdb->uDB,i) ) {
      alreadyAdded=true;
      break;
    }
  }
  
  if(!alreadyAdded)
    ts_vec_push_back( cdb->uDB, vec_ith(cl->lits,0) );
}


/*******************Make this Thread safe****************************/
void insert_binary_clause(ClauseCB* cdb, Clause *cl){
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

  list = vec_ith(cdb->bDB, not_l1);
  /*Search to see if element is already in list*/
  bool alreadyInList=false;
  Literal lInList;  
  for(i=0;i<ts_vec_size(list);i++){
    ts_vec_ith(lInList,list,i);
    if(lInList == l2){
      alreadyInList=true
    }
  }
  /*********************************************/

  /*Insert and update, if not previously inserted*/
  if(!alreadyInList){
    ts_vec_push_back(list,l2);
    //If it is not in list, it's not in the other list either
    list = vec_ith(cdb->bDB, not_l2);
    ts_vec_push_back(list,l1);
    //update clauseDB stats
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

  vec_literal_sort(cl->lits,cl->size);

  /*Check if alreadyAdded*/
  dassert(cdb->numTernaries == ts_vec_size(cdb->tDB));
  for(i=0;i<cdb->numTernaries;i++){
    ts_vec_ith(ternary,cdb->tDB,i);
    index=i;
    for(j=0;j<3;j++){
      if( ternary.lits[j] != vec_ith(cl->lits,j) ){
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
      ts_vec_ith(ternary,cdb->tDB,index);
      ts_vec_set_ith(ternary.flags,wId,true);
    }
    return;
  }
  /********************************************************/

  /*In case it doesn't exist, insert it and watch*/
  //Init flags
  ts_vec_init(ternary.flags,cdb->numWorkers);
  if(isOriginal){
    for(i=0;i<cdb->numWorkers;i++){
      ts_vec_set_ith(ternary.flags,i,true);
    }
  }else{
    for(i=0;i<cdb->numWorkers;i++){
      if(i==wId)
	ts_vec_set_ith(ternary.flags,i,true);
      else
	ts_vec_set_ith(ternary.flags,i,false);
    }
  }

  //Insert literals
  for(i=0;i<3;i++){
    ternary.lits[i]=vec_ith(cl->lits,i);
  }
  //update clauseDB
  ts_vec_push_back(cdb->tDB,ternary);

  /*add this clause position in tDB to each neg literal watches*/
  unsigned int litIndex;
  unsigned int indexOfNewClause=ts_vec_size(cdb->tDB)-1;
  for(i=0;i<3;i++){
    litIndex = lit_as_uint(-ternary.lits[i]);
    vec_push_back( vec_ith(cdb->3Watches, litIndex), indexOfNewClause );
  }

  cdb->numTernaries++;
  cdb->numClauses++;
  dassert(ts_vec_size(cdb->tDB)==cdb->numTernaries);
  /************************************************/
}


/****MAKE THIS THREAD SAFE**********/
unsigned int insert_nary_clause(ClauseCB* cdb, Clause *cl, bool isOriginal, unsigned int wId){
  //we will have problems here for keeping the same search for each thread
  dassert(cl->size > 3);
  int i,j;
  int index=-1;
  NClause nclause;
  vec_literal_sort(cl->lits,cl->size);

  /*Check if alreadyAdded*/
  dassert(cdb->numNClauses == ts_vec_size(cdb->nDB));
  for(i=0;i<cdb->numNClauses;i++){
    ts_vec_ith(nclause,cdb->tDB,i);

    if(vec_size(nclause.lits) == cl->size){
      index=i;
      for(j=0;j<cl->size;j++){
	if( vec_ith(nclause.lits,j) != vec_ith(cl->lits,j) ){
	  index = -1;
	  break;
	}
      }
    }

    if(index>0) break;
  }
  /*********************/

  /*In case the clause already exists, modify flags if it's learned*/
  if(index!=-1){
    if(!isOriginal){
      ts_vec_ith(nclause,cdb->tDB,index);
      ts_vec_set_ith(nclause.flags,wId,true);
    }
    return index;
  }
  /******************************************************************/

  /***********In case it doesn't exist, insert it***/
  //Init flags
  ts_vec_init(nclause.flags,cdb->numWorkers);
  if(isOriginal){
    for(i=0;i<cdb->numWorkers;i++){
      ts_vec_set_ith(nclause.flags,i,true);
    }
  }else{
    for(i=0;i<cdb->numWorkers;i++){
      if(i==wId)
	ts_vec_set_ith(nclause.flags,i,true);
      else
	ts_vec_set_ith(nclause.flags,i,false);
    }
  }
  //Insert literals
  for(i=0;i<cl->size;i++){
    vec_ith( nclause.lits,i ) = vec_ith(cl->lits,i);
  }
  nclause.isOriginal = isOriginal;

  //update clauseDB
  ts_vec_push_back(cdb->nDB,nclause);
  cdb->numNClauses++;
  cdb->numClauses++;
  dassert(ts_vec_size(cdb->nDB)==cdb->numNClauses);
  return cdb->numNClauses-1;
  /**************************************************/
}

#endif /* _CLAUSEDB_C_ */
