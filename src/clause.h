#ifndef _CLAUSE_H_
#define _CLAUSE_H_

#include "common.h"
#include "literal.h"
#include "kvec.h"

typedef struct _naryclause { /*half a cacheline per ternary clause*/ 
  //  unsigned int    size; //we don't need size, we have it in kvec_t(Literals)
  bool            is_original; /*yes=input clause, no=lemma*/
  bool            flags[18];   //supports max 14 threads
  kvec_t(Literal) lits;
} NClause; 

typedef struct _teryclause {/*half a cacheline per ternary clause*/ 
  Literal lits[3];
  bool    flags[18]; //supports max 14 threads
  char    padding[1]; 
} TClause; 

typedef struct _clause {
  kvec_t(Literal) lits;
  unsigned int size;
} Clause; /*Can be unit, binary, ternary or nary*/

typedef struct _naryThreadClause{ //32 bytes per thnClause
  unsigned int       size; //we have this info in the vector of literals
  unsigned int       activity; 
  Literal            lwatch1;
  Literal            lwatch2;
  void*              nextWatched1;
  void*              nextWatched2;
  kvec_t(Literal)*   lits;
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
