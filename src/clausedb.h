#ifndef _CLAUSEDB_H_
#define _CLAUSEDB_H_

#include <stdlib.h>
#include "clause.h"
#include "mergeSort.h"
#include "ts_vec.h"

#define MAX_RANDOM_NUMBERS     10000000
#define MAX_TERNARY_CLAUSES    10000000
#define MAX_NARY_CLAUSES       20000000
#define MAX_NARYTHREAD_CLAUSES 15000000


typedef struct _cdb {
  unsigned int           numVars;
  unsigned int           numWorkers;
  unsigned int           numClauses;
  unsigned int           numUnits;
  unsigned int           numOriginalUnits;
  unsigned int           numBinaries;
  unsigned int           numOriginalBinaries;
  unsigned int           numTernaries;
  unsigned int           numOriginalTernaries;
  unsigned int           numNClauses;
  unsigned int           numOriginalNClauses;
  unsigned int           numInputClauses;

  unsigned int           randomNumbers[MAX_RANDOM_NUMBERS];
  ts_vec_t(Literal)      uDB;  //this should be a thread_safe vector of Literals
  kvec_t(ts_vec_t(Literal))  bDB;  //this is a read only vector of BinaryLists (the lists should be thread safe)
  ts_vec_t(TClause)        tDB;  //this should be a thread_safe vector of TClause
  ts_vec_t(NClause)        nDB;  //this should be a thread_safe vector of NClause
  kvec_t(ts_vec_t(TClause*)) ternaryWatches;
} ClauseDB;

/**/
ClauseDB* init_clause_database(unsigned int numVars, unsigned int nWorkers);

/**/
unsigned int add_input_literal(ClauseDB* cdb, Literal lit);
void insert_unitary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int lastThUnit);
void insert_binary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int thLast1, unsigned int thLast2);
void insert_ternary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, int wId, TClause** ptrToTernary, unsigned int lastThTernary);
void insert_nary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int wId, NClause** ptrToNary, unsigned int lastThNary);
void cleanup_database(ClauseDB* cdb);
void vec_literal_sort(Clause *cl, unsigned int size);
void clause_database_resize_vectors(ClauseDB* cdb);
//void vec_literal_sort(Litera *lits, unsigned int size);

#endif /* _CLAUSEDB_H_ */
