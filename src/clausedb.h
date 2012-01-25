#ifndef _CLAUSEDB_H_
#define _CLAUSEDB_H_

#include "clause.h"
#include "binlist.h"

typedef struct _bincdb {
  BinaryList* bin_lists;
  
} BinaryClauseDB;

typedef struct _tercdb {
  TClause** index;
  int last;
} TernaryClauseDB;

typedef struct _ncdb {
  NClause** index;
  int last;
} NaryClauseDB;

int index_clause(TClause* c);
int index_clause(NClause* c);

#endif /* _CLAUSEDB_H_ */
