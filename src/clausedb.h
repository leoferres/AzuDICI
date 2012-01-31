#ifndef _CLAUSEDB_H_
#define _CLAUSEDB_H_

#include "clause.h"
#include "binlist.h"

typedef struct _cdb {
  unsigned int           numVars;
  unsigned int           numWorkers;
  unsigned int           numClauses;
  unsigned int           numUnits;
  unsigned int           numBinaries;
  unsigned int           numTernaries;
  unsigned int           numNClauses;
  unsigned int           numInputClauses;

  ts_vec(Literal)        uDB;  //this should be a thread_safe vector of Literals
  vec(ts_vec(Literal))   bDB;  //this is a read only vector of BinaryLists (the lists should be thread safe)
  ts_vec(TClause)        tDB;  //this should be a thread_safe vector of TClause
  ts_vec(NClause)        nDB;  //this should be a thread_safe vector of NClause
  vec(vec(unsigned int)) 3Watches;
} ClauseDB;

/**/
ClauseDB* init_clause_database(unsigned int numVars);

/**/
unsigned int add_input_literal(ClauseDB* cdb, Clause *cl);
void insert_unitary_clause(ClauseCB* cdb, Clause *cl);
void insert_binary_clause(ClauseDB* cdb, Clause *cl);
void insert_ternary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, int wId);
void insert_nary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, int wId);
void cleanup_database(ClauseDB* cdb);

#endif /* _CLAUSEDB_H_ */
