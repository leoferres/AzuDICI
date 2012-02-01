#ifndef _CLAUSE_H_
#define _CLAUSE_H_

#include "common.h"
#include "literal.h"
#include "kvec.h"
#include "ts_vec.h"

typedef struct _naryclause { /*half a cacheline per ternary clause*/ 
  unsigned int    size;
  bool            is_original; /*yes=input clause, no=lemma*/
  bool            flags[14];   //supports max 14 threads
  kvec_t(Literal) lits; 
} NClause; 

typedef struct _teryclause {/*half a cacheline per ternary clause*/ 
  Literal lits[3];
  bool    flags[14]; //supports max 14 threads
  char    padding[5]; 
} TClause; 

typedef struct _clause {
  kvec_t(Literal) lits;
  unsigned int size;
} Clause; /*Can be unit, binary, ternary or nary*/

typedef struct _naryThreadClause{
  unsigned int size;
  double       activity; 
  Literal      lwatch1;
  Literal      lwatch2;
  Literal      cachedLit1;
  Literal      cachedLit2;
  unsigned int posInDB;
}ThNClause;


#endif /* _CLAUSE_H_ */
