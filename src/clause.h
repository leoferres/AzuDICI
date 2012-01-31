#ifndef _CLAUSE_H_
#define _CLAUSE_H_

#include "common.h"
#include "literal.h"
#include "kvec.h"
#include "ts_vec.h"

typedef struct _naryclause {
  ts_vec(bool)  flags; //thread safe vector of literals
  kvec(Literal) lits; 
  //unsigned int size;
  bool is_original; /*yes=input clause, no=lemma*/
  char padding[3];  /*half a cacheline per n-ary clause*/ //RECALCULATE THIS DEPENDING ON ts_vec size
} NClause; 

typedef struct _teryclause {
  ts_vec(bool) flags; //thread safe vector of literals
  Literal lits[3];
  char padding[4]; /*half a cacheline per ternary clause*/ //RECALCULATE THIS DEPENDING ON ts_vec size
} TClause; 

typedef struct _clause {
  kvec(Literal) lits;
  unsigned int size;
} Clause; /*Can be unit, binary, ternary or nary*/

#endif /* _CLAUSE_H_ */
