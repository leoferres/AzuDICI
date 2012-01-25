#ifndef _CLAUSE_H_
#define _CLAUSE_H_

#include "common.h"
#include "literal.h"

typedef struct _naryclause {
  bool* flags;
  Literal* lits;
  unsigned int size;
  bool is_original; /*yes=input clause, no=lemma*/
  char padding[3]; /*half a cacheline per n-ary clause*/
} NClause; 

typedef struct _teryclause {
  Literal lits[3];
  char padding[4]; /*half a cacheline per ternary clause*/
} TClause; 

#endif /* _CLAUSE_H_ */
