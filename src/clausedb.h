#ifndef _CLAUSEDB_H_
#define _CLAUSEDB_H_

#include "clause.h"
#include "binlist.h"

typedef struct _cdb {
  ts_vec(Literal)     uDB;  //this should be a thread_safe vector of Literals
  ts_vec(BinaryList)  bDB;  //this should be a thread_safe vector of BinaryLists
  ts_vec(TClause)     tDB;  //this should be a thread_safe vector of TClause
  ts_vec(NClause)     nDB;  //this should be a thread_safe vector of NClause
} ClauseDB;

/**/
ClauseDB* init_clause_database(unsigned int numVars);

/**/
unsigned int add_input_literal(ClauseDB* cdb, Clause *cl);
void insert_unitary_clause(ClauseCB* cdb, Clause *cl);
void insert_binary_clause(ClauseDB* cdb, Clause *cl);
void insert_ternary_clause(ClauseDB* cdb, Clause *cl);
void insert_nary_clause(ClauseDB* cdb, Clause *cl, bool isOriginal);
void cleanup_database(ClauseDB* cdb);

#endif /* _CLAUSEDB_H_ */
