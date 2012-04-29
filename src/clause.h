#ifndef _CLAUSE_H_
#define _CLAUSE_H_

#include "common.h"
#include "literal.h"
#include "kvec.h"

typedef struct _naryclause { /*half a cacheline per ternary clause*/ 
  //unsigned int    size;
  Literal         *lits;  //lit[0] will keep size
  bool            flags[11];   //supports max 11 threads
  bool            is_original; /*yes=input clause, no=lemma*/
} NClause; 

typedef struct _teryclause {/*half a cacheline per ternary clause*/ 
  Literal lits[3];
  bool    flags[11]; //supports max 11 threads
  char    padding[8]; 
} TClause; 

typedef struct _clause {
  kvec_t(Literal) lits;
  unsigned int size;
} Clause; /*Can be unit, binary, ternary or nary*/

typedef struct _naryThreadClause{ //32 bytes per thnClause
  unsigned int       activity; 
  Literal            lwatch1;
  Literal            lwatch2;
  //unsigned int       size;
  Literal            cachedLit; //This won't be used
  void*              nextWatched1;
  void*              nextWatched2;
  Literal*           lits;
  unsigned int       posInDB; //needed for clause cleanup
}ThNClause;

typedef struct _reasonStruct{
  unsigned int     size;
  //  unsigned int     reason;
  Literal          binLit;
  TClause*         tClPtr;
  ThNClause*       thNClPtr; //this we need to update activity of clause
}Reason;

Reason no_reason();

bool is_unit_clause(Reason r);


#endif /* _CLAUSE_H_ */
