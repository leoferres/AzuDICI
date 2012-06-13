#ifndef _CLAUSEDB_H_
#define _CLAUSEDB_H_

#include <stdlib.h>
#include "clause.h"
#include "mergeSort.h"
#include "ts_vec.h"

#define MAX_RANDOM_NUMBERS     10000000
#define MAX_TERNARY_CLAUSES    0
#define MAX_NARY_CLAUSES       5000000
#define MAX_NARYTHREAD_CLAUSES 5000000
#define MIN_MEM_LIT            0


typedef struct _cdb {
  unsigned int               numVars;
  unsigned int               numWorkers;
  unsigned int               numClauses;
  unsigned int               numUnits;
  unsigned int               numOriginalUnits;
  unsigned int               numBinaries;
  unsigned int               numOriginalBinaries;
  unsigned int               numNClauses;
  unsigned int               numOriginalNClauses;
  unsigned int               numInputClauses;
  bool                       solved;

  unsigned int               randomNumbers[MAX_RANDOM_NUMBERS];
  kvec_t(Literal)            uDB;  
  kvec_t(kvec_t(Literal))    bDB; //THIS IS NOT THREAD SAFE
  kvec_t(NClause)            nDB;
} ClauseDB;

/**/
ClauseDB* init_clause_database(unsigned int numVars, unsigned int nWorkers);

/**/
unsigned int add_input_literal(ClauseDB* cdb, Literal lit);
void insert_unitary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int lastThUnit);
void insert_binary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int thLast1);
void insert_nary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int wId, NClause** ptrToNary, unsigned int lastThNary);
void cleanup_database(ClauseDB* cdb);
void vec_literal_sort(Clause *cl, unsigned int size);
bool is_done(ClauseDB* cdb);
void set_done(ClauseDB* cdb);
#endif /* _CLAUSEDB_H_ */
