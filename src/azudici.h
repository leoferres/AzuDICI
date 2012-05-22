#ifndef _AZUDICI_H_
#define _AZUDICI_H_

#include "clausedb.h"
#include "model.h"
#include "heap.h"
#include "strategy.h"
#include "stats.h"
#include "literal.h"

typedef struct _azuDICI{
  unsigned int        wId;
  unsigned int        lastUnitAdded;    //will be used in later versions
  unsigned int        lastNaryAdded;    //will be used in later versions
  unsigned int        lastTernaryAdded;
  unsigned int        lastBinaryAdded;
  unsigned int        randomNumberIndex;
  unsigned int        dlToBackjump;
  unsigned int        dlToBackjumpPos;
  //unsigned int        decisionLevel; This will go to model
  double              scoreBonus; //Remember to initialize this.

  BinNode**             localBinaries;
  kvec_t(unsigned int)  lastBinariesAdded;
  kvec_t(bool)          varMarks;
  kvec_t(ThNClause)     thcdb; //thread clause data base
  kvec_t(ThNClause*)    watches;

  Clause              conflict;
  Clause              lemma;
  Clause              lemmaToLearn; //The shortened lemma
  Clause              reasonToResolve;
  Reason              rUIP;
  ClauseDB            *cdb;

  Model               model; 
  MaxHeap             heap;
  Stats               stats; //To implement
  Strategy            strat; //To implement
  unsigned int        currentRestartLimit;
  unsigned int        currentCleanupLimit;

} AzuDICI;

/*Reserva de memoria para las estructuras de AzuDICI*/
AzuDICI* azuDICI_init(ClauseDB* generalClauseDB, unsigned int wID);

/*solve methods*/
unsigned int azuDICI_solve(AzuDICI* ad);
bool azuDICI_propagate(AzuDICI* ad);
void azuDICI_conflict_analysis(AzuDICI* ad);
void azuDICI_lemma_shortening(AzuDICI* ad);
void azuDICI_learn_lemma(AzuDICI* ad);
void azuDICI_backjump_to_dl(AzuDICI* ad, unsigned int dl);
bool azuDICI_set_true_uip(AzuDICI* ad);
bool azuDICI_clause_cleanup_if_adequate(AzuDICI* ad); //to implement
void azuDICI_restart_if_adequate(AzuDICI* ad); //to implement
Literal  azuDICI_decide(AzuDICI* ad);
void  azuDICI_increaseScore(AzuDICI* ad, Literal lit);
/****************/

/*Other methods*/
void azuDICI_init_thcdb(AzuDICI* ad); //to implement
bool azuDICI_propagate_w_binaries(AzuDICI* ad, Literal l);
bool azuDICI_propagate_w_n_clauses(AzuDICI* ad, Literal l);
void azuDICI_get_clause_from_reason(Clause *cl, Reason r, Literal l); //¿in clause.c?
void azuDICI_increase_activity(Reason r); //¿in clause.c?
void azuDICI_sort_lits_according_to_DL_from_index(Model m, Clause cl, unsigned int indexFrom); //¿in model.c? 
void azuDICI_notify_unassigned_lit(AzuDICI *ad, Literal l);
//unsigned int azuDICI_set_true_propagation(AzuDICI* ad, Literal l, Reason r);
ThNClause* azuDICI_insert_th_clause(AzuDICI* ad, Clause lemmaToLearn, NClause *ptrToNClause);
bool azuDICI_set_true_units(AzuDICI *ad);
void azuDICI_init_thcdb(AzuDICI* ad);
void azuDICI_print_clause(AzuDICI* ad, Clause cl);
void azuDICI_compact_and_watch_thdb(AzuDICI* ad);

#endif /* _AZUDICI_H_ */
