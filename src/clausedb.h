#ifndef _CLAUSEDB_H_
#define _CLAUSEDB_H_

#include <stdlib.h>
#include "clause.h"
#include "mergeSort.h"
#include "ts_vec.h"

#define MAX_RANDOM_NUMBERS     10000000
#define MAX_NARY_CLAUSES       5000000
#define MAX_NARYTHREAD_CLAUSES 5000000
#define MIN_MEM_LIT            300


typedef struct _cdb {
  unsigned int               numVars;
  unsigned int               numWorkers;
  unsigned int               numClauses;
  unsigned int               numUnits;
  unsigned int               numOriginalUnits;
  unsigned int               numBinaries;
  unsigned int               numOriginalBinaries;
  unsigned int               numTernaries;
  unsigned int               numOriginalTernaries;
  unsigned int               numNClauses;
  unsigned int               numOriginalNClauses;
  unsigned int               numInputClauses;

  unsigned int               randomNumbers[MAX_RANDOM_NUMBERS];
  Literal*                   uDB;  
  Literal**                  bDB; //THIS IS NOT THREAD SAFE
  unsigned int*              bListsSize;
  kvec_t(NClause)            nDB;
} ClauseDB;

/**/
ClauseDB* init_clause_database(unsigned int numVars, unsigned int nWorkers);

/**/
unsigned int add_input_literal(ClauseDB* cdb, Literal lit);
void insert_unitary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int lastThUnit);
void insert_binary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int thLast1, unsigned int thLast2);
void insert_nary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal, unsigned int wId, NClause** ptrToNary, unsigned int lastThNary);
void cleanup_database(ClauseDB* cdb);
void vec_literal_sort(Clause *cl, unsigned int size);

#endif /* _CLAUSEDB_H_ */
