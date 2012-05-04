#ifndef _CLAUSE_H_
#define _CLAUSE_H_

#include "common.h"
#include "literal.h"
#include "kvec.h"

#define CACHE_LINE_SIZE 64

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

//The fist two literals of the list are the two watched ones
typedef struct _naryThreadClause{//at least 29 bytes per thnClause
  unsigned int       activity; 
  unsigned int       size;
  bool               isOriginal;
  //Literal            lwatch1;
  //Literal            lwatch2;
  void*              nextWatched1;
  void*              nextWatched2;
  //  unsigned int       posInDB; //needed for clause cleanup
  Literal            lits[3]; //minimum amount of lits
}ThNClause;

typedef struct _reasonStruct{
  unsigned int     size;
  //  unsigned int     reason;
  Literal          binLit;
  TClause*         tClPtr;
  ThNClause*       thNClPtr; //this we need to update activity of clause
}Reason;

typedef struct _binNode{ //one cache line
  void* nextNode;
  Literal litList[(CACHE_LINE_SIZE/4)-1];
}BinNode;

typedef struct _binList{
  unsigned int size;
  unsigned int posInLastNode;
  BinNode *firstNode;
  BinNode *lastNode;
}BinList;

Reason no_reason();

bool is_unit_clause(Reason r);


#endif /* _CLAUSE_H_ */
