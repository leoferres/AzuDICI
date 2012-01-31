#ifndef _CLAUSEDB_C_
#define _CLAUSEDB_C_

#include "clausedb.h"

Clause inputClause;
unsigned int nInsertedClauses;

ClauseDB* init_clause_database(unsigned int nVars){

  ts_vec_init(inputClause.lits,nVars); //init inputClause
  inputClause.size = 0;
  nInsertedClauses=0;

  ClauseDB* cdb;
  ts_vec_init(cdb->uDB,0); //init vector with 0 elements

  ts_vec_init(cdb->bDB,2*(nVars+1));//init vector with 2*(nVars+1) elements
  /*Init each of this elements*/
  for(i=0;i<=2*(nVars+1);i++){
    init_binary_list( ts_vec_ith(cdb->bDB,i) );
  }

  ts_vec_init(cdb->tDB,0); //init vector with 0 elements

  ts_vec_init(cdb->nDB,0); //init vector with 0 elements
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
      insert_ternary_clause(cdb, &inputClause);
      break;
    default:
      insert_nary_clause(cdb, &inputClause, true);
    }    
    //new clause will be read
    inputClause.size = 0;
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
    if( ts_vec_ith(cl->lits,0) == ts_vec_ith(cdb->uDB,i) ) {
      alreadyAdded=true;
      break;
    }
  }
  
  if(!alreadyAdded)
    ts_vec_push_back( cdb->uDB, ts_vec_ith(cl->lits,0) );
}

void insert_binary_clause(ClauseCB* cdb, Clause *cl){
  //we will have problems here for keeping the same search for each thread
  dassert(cl->size == 2);

  unsigned int l1,l2;
  l1 = lit_as_uint(ts_vec_ith(cl->lits,0));
  l2 = lit_as_uint(ts_vec_ith(cl->lits,1));
  
  BinaryList list = ts_vec_ith
}

void insert_ternary_clause(ClauseCB* cdb, Clause *cl)
{
  //we will have problems here for keeping the same search for each thread
  dassert(cl->size == 3);
}

void insert_nary_clause(ClauseCB* cdb, Clause *cl, bool is Original){
  //we will have problems here for keeping the same search for each thread
  dassert(cl->size > 3);
}


#endif /* _CLAUSEDB_C_ */
