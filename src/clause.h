#ifndef _CLAUSE_H_
#define _CLAUSE_H_

#include "common.h"
#include "literal.h"

typedef struct _naryclause {
  bool* flags;
  ts_vec(Literal) lits; //thread safe vector of literals
  //unsigned int size;
  bool is_original; /*yes=input clause, no=lemma*/
  char padding[3];  /*half a cacheline per n-ary clause*/ //RECALCULATE THIS DEPENDING ON ts_vec size
} NClause; 

typedef struct _teryclause {
  Literal lits[3];
  char padding[4]; /*half a cacheline per ternary clause*/
} TClause; 

typedef struct _clause {
  ts_vec(Literal) lits;
  unsigned int size;
} Clause; /*Can be unit, binary, ternary or nary*/

#endif /* _CLAUSE_H_ */
