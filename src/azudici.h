#ifndef _SOLVER_H_
#define _SOLVER_H_

#include "clausedb.h"

typedef struct _azuDICI{
  unsigned int lastUnitAdded; //this will be used in later versions of the solver
  unsigned int lastTernaryAdded;
  unsigned int lastNAryAdded;
  vec(unsigned int) lastBinariesAdded;
  State           state;
  Watches         watches;
  Model           model;
  Heap            heap;
  Clause          conflict;
  Clause          newLemma;
  ClauseDB        *generalClauseDB;
  ThreadClauseDB  *selfClauseDB;
  Stats           stats;
} AzuDICI;

/*Reserva de memoria para las estructuras de AzuDICI*/
AzuDICI* azuDICI_init(ClauseDB* generalClauseDB);

/*Funciones para la resolución de SAT*/
unsigned int azuDICI_solve(AzuDICI* ad);
unsigned int azuDICI_propagate(AzuDICI* ad);
void azuDICI_conflict_analysis(AzuDICI* ad);
void azuDICI_lemma_shortening(AzuDICI* ad);
void azuDICI_learn_lemma(AzuDICI* ad);
void azuDICI_clause_cleanup(AzuDICI* ad);
void azuDICI_restart(AzuDICI* ad);
Lit  azuDICI_decide(AzuDICI* ad);
void azuDICI_backjump_to_dl(AzuDICI* ad, unsigned int dl);

/*Funciones utilitarias*/
unsigned int set_true_due_to_decision(AzuDICI* ad, Lit l);
unsigned int set_true_due_to_propagation(AzuDICI* ad, Lit l, Reason r);


#endif /* _SOLVER_H_ */
