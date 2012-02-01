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

  kvec_t(int)              randomNumbers;
  ts_vec_t(Literal)        uDB;  //this should be a thread_safe vector of Literals
  kvec_t(ts_vec_t(Literal))  bDB;  //this is a read only vector of BinaryLists (the lists should be thread safe)
  ts_vec_t(TClause)        tDB;  //this should be a thread_safe vector of TClause
  ts_vec_t(NClause)        nDB;  //this should be a thread_safe vector of NClause
  kvec_t(kvec_t(unsigned int)) ThreeWatches;
} ClauseDB;

/**/
ClauseDB* init_clause_database(unsigned int numVars);

/**/
unsigned int add_input_literal(ClauseDB* cdb, Clause *cl);
void insert_unitary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal);
void insert_binary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal);
void insert_ternary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, int wId);
void insert_nary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, int wId);
void cleanup_database(ClauseDB* cdb);
void vec_literal_sort(Clause* cl, unsigned int size);

#endif /* _CLAUSEDB_H_ */
