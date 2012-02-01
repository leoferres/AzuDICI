#ifndef _AZUDICI_H_
#define _AZUDICI_H_

#include "clausedb.h"
#include "threadclausedb.h"

typedef struct _azuDICI{
  unsigned int        lastUnitAdded; //will be used in later versions
  unsigned int        lastNaryAdded;  //will be used in late versions
  unsigned int        randomNumberIndex;
  unsigned int        dlToBackjump;
  unsinged int        dlToBackjumpPos;
  unsigned int        decisionLevel;
  double              scoreBonus; //Remember to initialize this.

  kvec(unsigned int)  lastBinariesAdded;
  kvec(bool)          varMarks;

  Reason              rUIP;
  State               state;
  Watches             watches;
  Model               model;
  Heap                heap;
  Clause              conflict;
  Clause              lemma;
  Clause              lemmaToLearn; //The shortened lemma
  ClauseDB            *cdb;
  ThreadClauseDB      *tcdb;
  Stats               stats;
} AzuDICI;

/*Reserva de memoria para las estructuras de AzuDICI*/
AzuDICI* azuDICI_init(ClauseDB* generalClauseDB);

/*Funciones para la resolución de SAT*/
unsigned int azuDICI_solve(AzuDICI* ad);
unsigned int azuDICI_propagate(AzuDICI* ad);
void azuDICI_conflict_analysis(AzuDICI* ad);
void azuDICI_lemma_shortening(AzuDICI* ad);
void azuDICI_learn_lemma_backjump_and_set_uip(AzuDICI* ad);
void azuDICI_clause_cleanup(AzuDICI* ad);
void azuDICI_restart(AzuDICI* ad);
Lit  azuDICI_decide(AzuDICI* ad);
void azuDICI_backjump_to_dl(AzuDICI* ad, unsigned int dl);

/*Funciones utilitarias*/
unsigned int set_true_due_to_decision(AzuDICI* ad, Lit l);
unsigned int set_true_due_to_propagation(AzuDICI* ad, Lit l, Reason r);


#endif /* _AZUDICI_H_ */
