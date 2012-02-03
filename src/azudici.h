#ifndef _AZUDICI_H_
#define _AZUDICI_H_

#include "clausedb.h"
//#include "threadclausedb.h" //to be implemented

typedef struct _azuDICI{
  unsigned int        wId;
  unsigned int        lastUnitAdded;    //will be used in later versions
  unsigned int        lastNaryAdded;    //will be used in later versions
  unsigned int        randomNumberIndex;
  unsigned int        dlToBackjump;
  unsigned int        dlToBackjumpPos;
  //unsigned int        decisionLevel; This will go to model
  double              scoreBonus; //Remember to initialize this.

  kvec_t(unsigned int)  lastBinariesAdded;
  kvec_t(bool)          varMarks;
  kvec_t(ThNClause)     thcdb; //thread clause data base
  kvec_t(ThNClause*)    watches;

  Clause              conflict;
  Clause              lemma;
  Clause              lemmaToLearn; //The shortened lemma
  Reason              rUIP;
  ClauseDB            *cdb;

  Model               model; 
  MaxHeap             heap;
  Stats               stats; //To implement
  Strategy            strat; //To implement
} AzuDICI;

/*Reserva de memoria para las estructuras de AzuDICI*/
AzuDICI* azuDICI_init(ClauseDB* generalClauseDB);

/*solve methods*/
unsigned int azuDICI_solve(AzuDICI* ad);
bool azuDICI_propagate(AzuDICI* ad);
void azuDICI_conflict_analysis(AzuDICI* ad);
void azuDICI_lemma_shortening(AzuDICI* ad);
void azuDICI_learn_lemma(AzuDICI* ad);
void azuDICI_backjump_to_dl(AzuDICI* ad, unsigned int dl);
void azuDICI_set_true_uip(AzuDICI* ad);
void azuDICI_clause_cleanup_if_adequate(AzuDICI* ad); //to implement
void azuDICI_restart_if_adequate(AzuDICI* ad); //to implement
Literal  azuDICI_decide(AzuDICI* ad);
/****************/

/*Other methods*/
void azuDICI_init_thcdb(AzuDICI* ad); //to implement
bool azuDICI_propagate_w_binaries(AzuDICI* ad, Literal l);
bool azuDICI_propagate_w_ternaries(AzuDICI* ad, Literal l);
bool azuDICI_propagate_w_n_clauses(AzuDICI* ad, Literal l);
Clause azuDICI_get_clause_from_reason(Reason r); //To implement, ¿in clause.c?
void azuDICI_increase_activity(Reason r); //To implement, ¿in clause.c?
void azuDICI_sort_lits_according_to_DL_from_index(Model m, Clause cl, unsigned int indexFrom); //To implement, ¿in model.c? 


unsigned int azuDICI_set_true_propagation(AzuDICI* ad, Literal l, Reason r);
ThNClause* azuDICI_insert_th_clause(AzuDICI* ad, Clause lemmaToLearn, bool isOriginal, unsigned int indexInDB);


#endif /* _AZUDICI_H_ */
