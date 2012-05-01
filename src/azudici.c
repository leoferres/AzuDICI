#ifndef _AZUDICI_C_
#define _AZUDICI_C_

#include "azudici.h"

pthread_rwlock_t cleanup_lock = PTHREAD_RWLOCK_INITIALIZER;

AzuDICI* azuDICI_init(ClauseDB* generalClauseDB, unsigned int wId){
  int i;

  AzuDICI *ad = (AzuDICI*)malloc(sizeof(AzuDICI));
  printf("Init of worker %d\n",wId);

  strategy_init(&(ad->strat), wId);  //strategy init according to wId

  ad->wId                 = wId;
  ad->cdb                 = generalClauseDB;
  //  ts_vec_size( dbSize, ad->cdb->uDB );
  ad->lastUnitAdded        = ad->cdb->numOriginalUnits;
  //  ts_vec_size( dbSize, ad->cdb->nDB );
  ad->lastBinaryAdded     = ad->cdb->numOriginalBinaries;
  ad->lastNaryAdded       = ad->cdb->numOriginalNClauses;
  ad->nVars               = ad->cdb->numVars;

  ad->randomNumberIndex   = 0;
  ad->dlToBackjump        = 0;
  ad->dlToBackjumpPos     = 0;
  ad->scoreBonus          = ad->strat.initialScoreBonus;
  ad->currentRestartLimit = ad->strat.initialRestartLimit;
  ad->currentCleanupLimit = ad->strat.initialCleanupLimit;

  //lastBinariesAdded
  //  kv_init( ad->lastBinariesAdded );
  //  kv_resize(unsigned int,  ad->lastBinariesAdded, 2*(ad->cdb->numVars+1) );
  //  kv_size(ad->lastBinariesAdded) = 2*(ad->cdb->numVars+1);
  //  int listSize;
  //  for( i=0; i < 2*(ad->cdb->numVars + 1) ; i++ ){
  //    listSize = kv_size(kv_A(ad->cdb->bDB, i) );
  //    kv_A(ad->lastBinariesAdded, i) = listSize;
  //  }

  //varMarks
  kv_init(ad->varMarks);
  kv_resize(bool, ad->varMarks, ad->cdb->numVars+1);
  kv_size(ad->varMarks) = ad->cdb->numVars+1;
  for( i=0; i < ad->cdb->numVars + 1 ; i++ ){
    kv_A(ad->varMarks, i) = false;
  }

  //thcdb (thread clause data base)
  kv_init(ad->thcdb);
  kv_resize(ThNClause*, ad->thcdb, MAX_NARYTHREAD_CLAUSES); //this is no longer necesary, but it is good to reserve space to avoid frequent memory reallocation

  //watches
  kv_init(ad->watches);
  kv_resize(ThNClause*,  ad->watches, 2*(ad->cdb->numVars + 1) );
  kv_size(ad->watches) = 2*(ad->cdb->numVars + 1);
  for( i=0; i < 2*(ad->cdb->numVars + 1) ; i++ ){
    kv_A(ad->watches, i) = NULL;
  }

  //conflict clause
  kv_init(ad->conflict.lits);
  kv_resize(Literal, ad->conflict.lits, ad->cdb->numVars);
  kv_size(ad->conflict.lits) = ad->cdb->numVars;
  ad->conflict.size = 0;

  //lemma clause
  kv_init(ad->lemma.lits);
  kv_resize(Literal, ad->lemma.lits, ad->cdb->numVars);
  kv_size(ad->lemma.lits) = ad->cdb->numVars;
  ad->lemma.size = 0;

  //shortenned lemma clause
  kv_init(ad->lemmaToLearn.lits);
  kv_resize(Literal, ad->lemmaToLearn.lits, ad->cdb->numVars);
  kv_size(ad->lemmaToLearn.lits) = ad->cdb->numVars;
  ad->lemmaToLearn.size = 0;

  model_init(ad->cdb->numVars, &ad->model);  //model

  //heap size depend on the strategy
  if(!ad->strat.decideOverLits)
    maxHeap_init(&(ad->heap), ad->cdb->numVars);
  else
    maxHeap_init(&(ad->heap), ad->cdb->numVars*2+1);

  stats_init(&(ad->stats));  //stats

  azuDICI_init_thcdb(ad); //here we link and watch to the general cdb->nDB

  return ad;
}

unsigned int azuDICI_solve(AzuDICI* ad){
  //setTrue at dl 0 all unit clauses
  //printf("Solve called for worker %d\n",ad->wId);
  if( !azuDICI_set_true_units(ad) ) return 20;
  //printf("Units set true\n");
  Literal dec;
  while (true) {
    while(!azuDICI_propagate(ad)){ //While conflict in propagate
      //printf("Conflict\n");
      if( ad->model.decision_lvl == 0 ) return 20;
      //printf("Do conflict analysis\n");
      //printf("Conflict is "); azuDICI_print_clause(ad,ad->conflict);
      azuDICI_conflict_analysis(ad);
      //printf("Lemma from conflict analysis: "); azuDICI_print_clause(ad,ad->lemma);
      //printf("Do lemma shortening\n");
      azuDICI_lemma_shortening(ad);
      //printf("Lemma from lemma shortening: "); azuDICI_print_clause(ad,ad->lemmaToLearn);
      //printf("Learn lemma\n");
      azuDICI_learn_lemma(ad);
      //printf("backjump to dl %d, \n",ad->dlToBackjump);
      azuDICI_backjump_to_dl(ad,ad->dlToBackjump);
      //printf("Set true uip\n");
      azuDICI_set_true_uip(ad);
      //printf("done set true\n");
    }
    azuDICI_restart_if_adequate(ad);
    if( !azuDICI_clause_cleanup_if_adequate(ad) ) return 20;
    
    //printf("Decide\n");
    dec = azuDICI_decide(ad);
    //printf("Worker %d decision is %d\n",ad->wId,dec);
    if (dec == 0){
      /*FOR DEBUG*/
      /* for(int i=1; i<= ad->cdb->numVars; i++){ */
      /* 	dassert(!model_is_undef(i,&ad->model)); */
      /* } */
      /**********/
      return 10;
    }
    //printf("Decision is %d\n",dec);
    model_set_true_decision( &ad->model, dec ); //decisionLevel should be increased
    ad->stats.numDecisions++;
  } 
  return 0;
}

bool azuDICI_propagate( AzuDICI* ad ){
  Literal lit;  // from highest priority to lowest
  lit=1;
  while ( lit != 0 ) {
    lit = model_next_lit_for_2_prop( &ad->model);
    //printf("2 prop with lit %d, from %d\n",lit,ad->cdb->numVars);
    if ( lit!= 0 ) {  
      if ( !azuDICI_propagate_w_binaries(ad, lit)) return false;
      else continue; 
    } 
    
    lit = model_next_lit_for_n_prop( &ad->model);
    //printf("n prop with lit %d\n",lit);
    if ( lit != 0 ) {  
      if( !azuDICI_propagate_w_n_clauses(ad, lit)) return false; 
      else continue; 
    } 
  }
  return true;
}


void azuDICI_conflict_analysis(AzuDICI* ad){
  ad->scoreBonus *= ad->strat.scoreBonusInflationFactor; //Lits activity not yet implemented

  ad->stats.numConflictsSinceLastCleanup++;
  ad->stats.numConflictsSinceLastRestart++;
  ad->stats.numConflictsSinceLastDLZero++;
  ad->stats.numConflicts++;

  ad->lemma.size = 0;
  
  unsigned int i, numLitsThisDLPending=0;
  Literal lit, poppedLit=0;
  unsigned int index=0;
  Clause cl = ad->conflict;
  Reason r;

  //printf("Current dl is %d\n",ad->model.decision_lvl);
  //printf("Conflict size is %d\n",cl.size);

  while(true){
    //Clause increaseActivity is done when processing the reason or when conflict detected......
    
    while( index < cl.size ){//traverse conflictingclause/currentreasonclause,
      lit = kv_A( cl.lits, index ); 
      //printf("Lit is %d\n",lit);
      index++; // marking literals and counting lits of this DL
      azuDICI_increaseScore(ad, lit);
      //printf("Score increased\n");
      if( lit != poppedLit ){
	//in reasonclause for lit, don't treat this "poppedLit" 
	if( model_lit_is_of_current_dl( lit, &ad->model ) ){
	  if ( !kv_A( ad->varMarks, var(lit) ) ){
	    kv_A( ad->varMarks, var(lit) ) = true;
	    numLitsThisDLPending++;
	    //printf("numLitsThisDLPendingi is %d\n",numLitsThisDLPending );
	  }
	} else if ( !kv_A(ad->varMarks, var(lit) ) && model_get_lit_dl(lit,&ad->model) > 0 ) {
	  kv_A(ad->varMarks, var(lit)) = true;  // Note: we ignore dl-zero lits
	  ad->lemma.size++;
	  kv_A(ad->lemma.lits, ad->lemma.size) = lit; // lower-dl-lit marked->already in lemma
	}
      }
    }

    do { // pop and unassign until finding next marked literal in the stack
      poppedLit = model_pop_and_set_undef(&ad->model); 
      dassert(poppedLit!=0);
      azuDICI_notify_unassigned_lit(ad, poppedLit);
      //printf("unassigned lit %d,notified\n",poppedLit);
    } while ( !kv_A(ad->varMarks, var(poppedLit)) );
    kv_A(ad->varMarks, var(poppedLit)) = false;

    //printf("numLitsThisDLPending = %d\n",numLitsThisDLPending);
    r = model_get_reason_of_lit(poppedLit , &ad->model);
    //printf("reason got\n");
    azuDICI_increase_activity(r);
    //printf("Reason activity increased\n");
    //conflict will be overwritten here
    azuDICI_get_clause_from_reason(&cl, r,poppedLit);
    //printf("Reason clause is: ");azuDICI_print_clause(ad, cl);

    if ( numLitsThisDLPending == 1) {
      break;  // i.e., only 1 lit this dl pending: this last lit is the 1UIP.
    }
    numLitsThisDLPending--;
    index=0;
  }

  dassert(numLitsThisDLPending==1);
  kv_A(ad->lemma.lits,0) = -poppedLit; //THe 1UIP is the first lit in the lemma
  ad->lemma.size++; 

  for (i=0;i<ad->lemma.size;i++) {
    kv_A( ad->varMarks, var(kv_A(ad->lemma.lits,i)) ) = false;
  }

  /*FOR DEBUG*/
  /*for(i=0; i<=ad->cdb->numVars; i++){
    if(kv_A( ad->varMarks, i)){
      printf("var %d keeps being marked\n",i);
      exit(-1);
    }
    }*/


}

void azuDICI_lemma_shortening(AzuDICI* ad){
  /* Try to mark more intermediate variables, with the goal to minimize
   * the conflict clause.  This is a BFS from already marked variables
   * backward through the implication graph.  It tries to reach other marked
   * variables.  If the search reaches an unmarked decision variable or a
   * variable assigned below the minimum level of variables in the first uip
   * learned clause, then the variable from which the BFS is started is not
   * redundant.  Otherwise the start variable is redundant and will
   * eventually be removed from the learned clause.
   */
  Clause cl;
  kv_init(cl.lits);
  kv_resize(Literal, cl.lits, 2*ad->nVars);
  unsigned int j, lastMarkedInLemma, testingIndex;
  Literal litOfLemma, testingLit;
  unsigned int i, lowestDL;
  Var v;
  bool litIsRedundant;
  Reason r;

  ad->lemmaToLearn.size = 0;
  if (ad->model.decision_lvl == 0) return;

  //printf("Mark vars\n");
  //First of all, mark all lits in original lemma.
  for ( i=0; i < ad->lemma.size; i++ ){ 
    //printf("marking var %d\n",var(kv_A(ad->lemma.lits,i)));
    kv_A( ad->varMarks, var(kv_A(ad->lemma.lits,i)) )=true;
  }

  //Order lemma's lit from most recent to oldest (from highest dl to lowest)
  //printf("about to order according to lit height\n");
  azuDICI_sort_lits_according_to_DL_from_index(ad->model, ad->lemma,1);
  //printf("ordered lemma:");azuDICI_print_clause(ad,ad->lemma);
  lowestDL = model_get_lit_dl(kv_A(ad->lemma.lits, ad->lemma.size-1), &ad->model);
  //printf("LowestDl is %d\n",lowestDL);

  ad->dlToBackjump = 0; 
  ad->dlToBackjumpPos=0; 

  //stats.totalLearnedLits+=numLitsInLemma;

  ad->lemmaToLearn.size = 0;

  //printf("Lemma size is %d\n",ad->lemma.size);
  //Go through the lits in lemma (except the UIP - lemma[0])
  // and test if they're redundant
  for( i=0; i < ad->lemma.size; i++ ){
    litIsRedundant=true;

    litOfLemma = kv_A(ad->lemma.lits,i);
    dassert (kv_A(ad->varMarks,var(litOfLemma)));

    //Begins test to see if literal is redundant
    if( model_lit_is_decision(litOfLemma,&ad->model) || i==0 ){
      //printf("Lit %d is not redundant\n",litOfLemma);
      //If it's decision or the UIP, lit is not Redundant 
      litIsRedundant=false;
    }else{
      //We add reasons' lits at the end of the lemma
      r = model_get_reason_of_lit(litOfLemma,&ad->model);
      //printf("getting reason of litOfLemma %d\n",litOfLemma);
      azuDICI_get_clause_from_reason(&cl, r,litOfLemma);
      //printf("Clause reason is ");azuDICI_print_clause(ad,cl);

      //add literals of lit's reason to test
      lastMarkedInLemma = ad->lemma.size;
      for( j=0 ; j<cl.size ; j++ ){
	v = var( kv_A(cl.lits, j));
	if(kv_A(ad->varMarks, v)) continue;

	kv_A(ad->varMarks, v) = true;
	//printf("marking var %d\n",v);
	kv_A(ad->lemma.lits, lastMarkedInLemma++) = kv_A(cl.lits,j);
      }

      //test added literals and subsequent ones....
      testingIndex = ad->lemma.size;
      //printf("testingIndex id %d, lemma size is %d and lastMarkedInLemma is %d\n",testingIndex,ad->lemma.size,lastMarkedInLemma);
      while(testingIndex < lastMarkedInLemma){
	testingLit = kv_A(ad->lemma.lits,testingIndex);
	//printf("testingLit is %d\n",testingLit);
	dassert(kv_A(ad->varMarks, var(testingLit)));
	if ( model_get_lit_dl(testingLit, &ad->model) < lowestDL || //has lower dl
	     model_lit_is_decision(testingLit, &ad->model) ) { //is decision
	  //test fails
	  litIsRedundant=false;
	  break;
	}
      
	//printf("getting reason of testingLit %d\n",-testingLit);
	r = model_get_reason_of_lit(-testingLit,&ad->model);
	azuDICI_get_clause_from_reason(&cl, r,-testingLit);
	//printf("Clause reason is ");azuDICI_print_clause(ad,cl);
      
	//If testing lit passes test, add its reason's literals to test
	for(j=0;j<cl.size;j++){
	  v = var( kv_A(cl.lits,j) );	
	  if(kv_A(ad->varMarks,v)) continue;
	  //printf("Marking var %d\n",v);
	  kv_A(ad->varMarks,v) = true;
	  kv_A(ad->lemma.lits,lastMarkedInLemma++) = kv_A(cl.lits, j);
	}
	testingIndex++;
      }
      /*FOR DEBUG*/
      if(testingIndex==lastMarkedInLemma){ dassert(litIsRedundant);}
      /***********/
      //Test ends only if a) Some unit or decision is reached
      //or b) all the reason's lits are already marked

      //Clean tested literals
      //printf("Clean marks\n");
      while ( lastMarkedInLemma > ad->lemma.size ) {
	kv_A(ad->varMarks,var(kv_A(ad->lemma.lits,--lastMarkedInLemma)))=false;
	//varMarks.setUnMarked(var(lemma[--lastMarkedInLemma]));
      }
    }

    //Add (or not) litOfLemma to lemmaToLearn
    if(litIsRedundant){
      kv_A(ad->lemma.lits,i)=0;
      kv_A(ad->varMarks,var(litOfLemma))=false;
    }else{
      //printf("adding lit %d to lemmaToLearn\n",litOfLemma);
      kv_A(ad->lemmaToLearn.lits, ad->lemmaToLearn.size) = litOfLemma;
      //Keep track of literal with highest dl besides de UIP
      if ( ad->lemmaToLearn.size > 0 && ad->dlToBackjump < model_get_lit_dl(litOfLemma, &ad->model) ){
	ad->dlToBackjump = model_get_lit_dl(litOfLemma, &ad->model);
	ad->dlToBackjumpPos = ad->lemmaToLearn.size;  //dlpos is position in lemma of maxDLLit
      }
      ad->lemmaToLearn.size++;
    }
  }

  //printf("LemmaTo learn size is %d\n",ad->lemmaToLearn.size);
  //printf("uip is %d\n",kv_A(ad->lemmaToLearn.lits,0));

  /*Put in lemmaToLearn[1] the lit with highest dl*/
  if(ad->lemmaToLearn.size>1){
    Literal aux = kv_A(ad->lemmaToLearn.lits, ad->dlToBackjumpPos);
    kv_A(ad->lemmaToLearn.lits, ad->dlToBackjumpPos) = 
      kv_A(ad->lemmaToLearn.lits, 1);
    kv_A(ad->lemmaToLearn.lits, 1) = aux;
  }
  //printf("LemmaTo learn size is %d\n",ad->lemmaToLearn.size);
  //printf("uip is %d\n",kv_A(ad->lemmaToLearn.lits,0));

  //  stats.totalRemovedLits += (numLitsInLemma - numLitsInLemmaToLearn);

  for (i=0;i<ad->lemmaToLearn.size;i++){
    kv_A(ad->varMarks, var(kv_A(ad->lemmaToLearn.lits,i)) )=false;
  }

  /*FOR DEBUG*/
  /*for(i=0; i<=ad->cdb->numVars; i++){
    if(kv_A( ad->varMarks, i)){
      printf("var %d keeps being marked\n",i);
      exit(-1);
    }
    }*/
  kv_destroy(cl.lits);
  /*  if(verboseMin)
    if (!(stats.numConflicts%10000))
    cout<<"total lits: "<<stats.totalLearnedLits<<" removed: "<<stats.totalRemovedLits<<"("<<(100*stats.totalRemovedLits)/stats.totalLearnedLits<<"pct)"<<endl;*/

}

void azuDICI_learn_lemma(AzuDICI* ad){
  //learn clause stored in ad->lemmaToLearn
  //store in ad->rUIP the reason to set the uip
  //If nclause, the 2 watched literals are the uip and the highest dl one.
  //printf("Learning lemma size %d: ",ad->lemmaToLearn.size); azuDICI_print_clause(ad,ad->lemmaToLearn);
  switch(ad->lemmaToLearn.size){
  case 1:
    azuDICI_insert_unitary_clause(ad, &ad->lemmaToLearn);
    ad->rUIP.size     = 1;
    ad->rUIP.binLit   = 0;
    ad->rUIP.tClPtr   = NULL;
    ad->rUIP.thNClPtr = NULL;
    dassert(ad->dlToBackjump==0);
    break;

  case 2:
    azuDICI_insert_binary_clause(ad, &ad->lemmaToLearn);
    ad->lastBinaryAdded++;
    ad->rUIP.size     = 2;
    ad->rUIP.binLit   = kv_A(ad->lemmaToLearn.lits,1);
    ad->rUIP.tClPtr   = NULL;
    ad->rUIP.thNClPtr = NULL;
    break;

  default:
    //printf("learning n clause\n");
    ;Literal uip    = kv_A(ad->lemmaToLearn.lits,0);
    Literal hdlLit = kv_A(ad->lemmaToLearn.lits,1);

    dassert(ad->lemmaToLearn.size>=3);

    /*put in lemmaToLearn[0] the uip */
    //highest decision level is ad->dlToBackjump   
    for(int i=1; i < ad->lemmaToLearn.size; i++){
      if( kv_A(ad->lemmaToLearn.lits,i) == uip ){
	kv_A(ad->lemmaToLearn.lits,i) = kv_A(ad->lemmaToLearn.lits,0);
	kv_A(ad->lemmaToLearn.lits,0) = uip;
	break;
      }
    }

    /*put in lemmaToLearn[1] literal with highest DL*/
    //highest decision level is ad->dlToBackjump   
    for(int i=1; i < ad->lemmaToLearn.size; i++){
      if( kv_A(ad->lemmaToLearn.lits,i) == hdlLit ){
	dassert(model_get_lit_dl(kv_A(ad->lemmaToLearn.lits,i),&ad->model) == ad->dlToBackjump);
	kv_A(ad->lemmaToLearn.lits,i) = kv_A(ad->lemmaToLearn.lits,1);
	kv_A(ad->lemmaToLearn.lits,1) = hdlLit;
	break;
      }
    }

    ThNClause *ptrToThNClause = 
      azuDICI_insert_th_clause(ad, ad->lemmaToLearn, false);

    ad->rUIP.size     = ad->lemmaToLearn.size;
    ad->rUIP.binLit   = 0;
    ad->rUIP.tClPtr   = NULL;
    ad->rUIP.thNClPtr = ptrToThNClause;
    //Nclause
  }
}

void azuDICI_backjump_to_dl(AzuDICI* ad, unsigned int dl){
  Literal lit;
  unsigned int dL1 = ad->model.decision_lvl;

  if (dl==0) ad->stats.numConflictsSinceLastDLZero=0;
  while ( dL1>dl ){
    lit = model_pop_and_set_undef(&ad->model);
    while ( lit != 0 ) {
      azuDICI_notify_unassigned_lit(ad,lit);
      lit=model_pop_and_set_undef(&ad->model);
    }
    dL1--;
  }
  ad->model.decision_lvl = dl;
}

void azuDICI_set_true_uip(AzuDICI* ad){
  if(ad->lemmaToLearn.size==1) {
    ad->stats.numDlZeroLitsSinceLastRestart++;
    ad->stats.numDLZeroLits++;
  }

  model_set_true_w_reason(kv_A(ad->lemmaToLearn.lits,0), ad->rUIP, &ad->model);
  //conflict ends, and then we propagate
  ad->conflict.size = 0;
}

bool azuDICI_clause_cleanup_if_adequate(AzuDICI* ad){
  if(ad->stats.numConflictsSinceLastCleanup >= ad->currentCleanupLimit){
    unsigned int numUnits = 0;
    unsigned int numBinaries = 0;
    unsigned int numTernaries = 0;
    unsigned int numDeletes = 0;
    unsigned int numRealDeletes = 0;
    unsigned int numTrueClauses = 0;
    
    azuDICI_backjump_to_dl(ad,  0 );
    dassert(ad->model.decision_lvl == 0);
    //update stats
    ad->stats.numConflictsSinceLastCleanup = 0;
    ad->stats.numCleanups++;
    
    ad->currentCleanupLimit *= ad->strat.cleanupMultiplier;
    
    //printf("CLEANUP\n");
    //Cleanup
    unsigned int i,j;
    Clause tentativeTernaryOrBinary;
    kv_init(tentativeTernaryOrBinary.lits);
    kv_resize(Literal, tentativeTernaryOrBinary.lits, ad->nVars);
    ThNClause *cl;
    bool delete;
    Reason r;
    
    r.size     = 1;
    r.binLit   = 0;
    r.tClPtr   = NULL;
    r.thNClPtr = NULL;
    
    //azucidi_cleanup_watches(ad);
    for( i=0; i < 2*(ad->nVars + 1) ; i++ ){
      kv_A(ad->watches, i) = NULL;
    }
    
    for(i=0;i<kv_size(ad->thcdb);i++){
      cl = kv_A(ad->thcdb, i);
      delete = true;
      //generalCl = &kv_A(ad->cdb->nDB, cl->posInDB);
      if( cl->isOriginal ||
 	  cl->activity >= ad->strat.activityThreshold || cl->size <= 3 ){ //keep it
	delete = false;
	
	unsigned int keptLits = 0;
 	tentativeTernaryOrBinary.size=0;
 	for(j=0; j<cl->size; j++){
 	  if(model_is_true(cl->lits[j], &ad->model)){
	    numDeletes++;
	    numTrueClauses++;
 	    delete = true; //it is true at dl 0, no need to have it.
 	    break;
 	  }else if(model_is_undef(cl->lits[j], &ad->model)){ //keep only false lits
	    cl->lits[keptLits++] = cl->lits[j];
 	    kv_A(tentativeTernaryOrBinary.lits,tentativeTernaryOrBinary.size++) 
	      = cl->lits[j];
	    //tentativeTernaryOrBinary.size++;
 	  }
 	}
	cl->size = keptLits;
	
 	if( !delete && keptLits <= 2 ){ //new unit or binary
	  numDeletes++;
 	  delete = true;
	  if(keptLits == 1 ){
	    numUnits++;
	    azuDICI_insert_unitary_clause(ad, &tentativeTernaryOrBinary);
	    if (model_is_undef(kv_A(tentativeTernaryOrBinary.lits,0), &ad->model)){
	      model_set_true_w_reason(kv_A(tentativeTernaryOrBinary.lits,0),r,&ad->model);
	      ad->stats.numDLZeroLits++;
	      ad->stats.numDlZeroLitsSinceLastRestart++;      
	    }else if(model_is_false(kv_A(tentativeTernaryOrBinary.lits,0),&ad->model)){
	      return false;
	    }
	  }else if(keptLits == 2){
	    numBinaries++;
	    azuDICI_insert_binary_clause(ad, &tentativeTernaryOrBinary );
	    ad->lastBinaryAdded++;
	  }
	}
      }else{
	numDeletes++;
	numRealDeletes++;
      }
      
      if(delete){
	cl->size = 0; //that's how we will know if is deleted
      }
    }
    
    azuDICI_compact_and_watch_thdb(ad);
    kv_destroy(tentativeTernaryOrBinary.lits);
    if(ad->wId==0 && ad->stats.numCleanups%10 == 0){
      printf("|                                   GENERAL STATS                               |              CLEANUP STATS                           |\n");
      printf("|WID|   nDecisions  |     nProps    |nConflicts|   units  |    bins  | nary_Cls | newUnits |  newBins |delTrueCls|   nDels  | nRealDels|\n");
      
    }
    
    printf("|%*d|%*lu|%*lu|%*d|%*d|%*d|%*d|%*d|%*d|%*d|%*d|%*d|\n", 
	   3,ad->wId, 15,ad->stats.numDecisions, 15,ad->stats.numProps, 
	   10,ad->stats.numConflicts, 10,ad->numUnits, 
	   10,ad->numBinaries, 10,ad->numNClauses,
	   10,numUnits, 10,numBinaries, 10,numTrueClauses, 
	   10,numDeletes, 10,numRealDeletes);
    return azuDICI_propagate(ad);
  }
  return true;
}

void azuDICI_compact_and_watch_thdb(AzuDICI* ad){
  uint lastValidPos=0;
  Literal l1, l2;
  ThNClause *cl;
  int i,j;
  for(i=0;i<kv_size(ad->thcdb);i++){
    cl = kv_A(ad->thcdb, i);
    if(cl->size > 0){//If kept, normalize activities
      dassert(cl->size>2);
      cl->activity = cl->activity/2;

      //remember cl.lits[0] stores size of nclause
      //select two undef lits to watch
      /*FOR DEBUG*/
      for(j=0;j<cl->size;j++){
	dassert(model_is_undef(cl->lits[j],&ad->model));
      }
      /************/

      l1 = cl->lits[0];
      l2 = cl->lits[1];

      cl->nextWatched1  = (void*)kv_A(ad->watches, lit_as_uint(l1)); 
      cl->nextWatched2  = (void*)kv_A(ad->watches, lit_as_uint(l2));

      kv_A(ad->thcdb,lastValidPos++) = cl; //copy it to last valid pos in thcdb

      kv_A(ad->watches, lit_as_uint(l1)) = cl;
      kv_A(ad->watches, lit_as_uint(l2)) = cl;
    }
  }
  kv_size(ad->thcdb) = lastValidPos;
}

void azuDICI_restart_if_adequate(AzuDICI* ad){
  if( ad->stats.numConflictsSinceLastRestart >= ad->currentRestartLimit ){
    //printf("Restart\n");
    ad->stats.numRestarts++;
    ad->stats.numDlZeroLitsSinceLastRestart = 0;
    ad->stats.numConflictsSinceLastRestart  = 0;
    ad->currentRestartLimit = strategy_get_next_restart_limit(&(ad->strat), ad->currentRestartLimit);
    azuDICI_backjump_to_dl(ad,  0 );
  }
}

Literal  azuDICI_decide(AzuDICI* ad){
  int i,v=0;
  Literal l;

  if ( (ad->model.decision_lvl < ad->strat.DLBelowWhichRandomDecisions) &&
       !(ad->stats.numDecisions % ad->strat.fractionRandomDecisions) ){
    unsigned int rNumber= ad->cdb->randomNumbers[ad->randomNumberIndex++];
    v = (unsigned int) ( (double)rNumber/RAND_MAX * ad->nVars ) + 1;
    for( i=1;i<1000;i++ ) {
      //printf("v is %d\n",v);
      if (model_is_undef_var(v,&ad->model)) break;
      v = (v+1) % ad->nVars +1;
    }

    //printf("random decision is %d and i is %d\n",v,i);
    if ( !model_is_undef_var(v,&ad->model) ) v=0;
  }

  //printf("random decision is %d\n",v);

  /*  if (ad->strat.decideOverLits){
    if(v!=0){
      unsigned int pol;
      pol= ad->cdb->randomNumbers[ad->randomNumberIndex++];
      printf("Worker %d, pol is %d\n",ad->wId, pol);
      printf("v is %d\n",ad->wId, pol);
      if( pol >= (RAND_MAX/2) ) l=v;
      else l = -v;
    }
    }*/


  if(ad->strat.decideOverLits){
    l=0;
    if (!v) 
      do {
	l = uint_as_lit(maxHeap_remove_max(&ad->heap));
	//printf("heap decision is %d\n",l);
	if (!l) return(0);
      } while (!model_is_undef_var(var(l),&ad->model));
    dassert( var(l) <= ad->nVars );
    return l;  
  }



  //Decide over vars depends on other strat parameters.  
  if (!v) do {
      v = maxHeap_remove_max(&ad->heap);
      //printf("heap decision is %d\n",v);
      if (!v) return(0);
    } while (!model_is_undef_var(v,&ad->model));


  dassert(model_is_undef_var(v,&ad->model)); 
  dassert(v <= ad->nVars );
  if( ad->strat.phaseSelectionDLParity ){
    if( ad->model.decision_lvl % 2 ) return -v;
    else return v;
  } else if( ad->strat.phaseSelectionLastPhase ){
    if( model_get_last_phase(v, &ad->model) ) return v;
    else return -v;
  } else if( ad->strat.phaseSelectionAlwaysPositive ){
    return v;
      }  else if( ad->strat.phaseSelectionAlwaysNegative ){
    return -v;
  }
  
  return -v;  // default is always negative
}

bool azuDICI_propagate_w_binaries(AzuDICI* ad, Literal l){
  Literal lToSetTrue;
  unsigned int sizeOfList, litIndex;
  litIndex = lit_as_uint(l);
  //  sizeOfList = kv_A(ad->lastBinariesAdded,litIndex); //hack for same search
  sizeOfList = kv_size(kv_A(ad->thbdb,litIndex));
  //  ts_vec_size(sizeOfList, list);
  Reason r;
  r.tClPtr = NULL;
  r.thNClPtr = NULL;

  for(int i=0;i<sizeOfList;i++){
    lToSetTrue= kv_A(kv_A(ad->thbdb,litIndex), i);
    if (model_is_undef(lToSetTrue,&ad->model)){
      r.size   = 2;
      r.binLit = -l;
      model_set_true_w_reason(lToSetTrue,r,&ad->model);
      ad->stats.numProps++;
      if(ad->model.decision_lvl ==0){
	ad->stats.numDLZeroLits++;
	ad->stats.numDlZeroLitsSinceLastRestart++;
      }
    }else if(model_is_false(lToSetTrue,&ad->model)){ //Conflict
      ad->conflict.size = 2;
      kv_A(ad->conflict.lits,0) = -l;
      kv_A(ad->conflict.lits,1) = lToSetTrue;
      return false;
    }    
  }
  return true;
}

bool azuDICI_propagate_w_n_clauses(AzuDICI* ad, Literal l){
  int i;
  //printf("npropagating lit %d\n",l);
  ThNClause** ptrToWatchedClauseAddr =  &kv_A(ad->watches,lit_as_uint(-l));
  ThNClause* watchedClause = kv_A(ad->watches,lit_as_uint(-l));
  bool first;
  unsigned int sizeOfClause;
  Literal otherLit, toReselect;
  Reason r;
  r.binLit = 0;
  r.tClPtr = NULL;
  bool foundForReselection;

  while(watchedClause){
    //printf("Visiting thn clause\n");
    //for(i=0;i<kv_size(watchedClause->lits[0]);i++){
    //printf("%d ", kv_A(watchedClause->lits[0],i));
    //}
    //printf("\n");

    if (watchedClause->lits[0] == -l){
      //printf("Is first watch %d\n",watchedClause->lwatch1);
      first=true;
      otherLit = watchedClause->lits[1];
    } else{
      //printf("Is second watch  %d\n", watchedClause->lwatch2);
      dassert(watchedClause->lits[1] == -l);
      first=false;
      otherLit = watchedClause->lits[0];
    }

    if(model_is_true(otherLit,&ad->model) ){
      //Update watchedLiteral and previouslyWatchedLiteral
      if(first){
	ptrToWatchedClauseAddr =(ThNClause**)&(watchedClause->nextWatched1);
	//watchedClause = watchedClause->nextWatched1; see below, is the same
      }else{
	ptrToWatchedClauseAddr =(ThNClause**)&(watchedClause->nextWatched2);
	//watchedClause = watchedClause->nextWatched2; see below, is the same
      }
      watchedClause = *ptrToWatchedClauseAddr;
      //__builtin_prefetch (watchedClause->lits->a, 0, 0);
      //printf("Watched clause is %d\n",watchedClause);
      continue;
    }

      //if other watched is not true, try to reselect.
    //printf("with clause with size %d: ",sizeOfClause);
    //for(i=0;i<sizeOfClause;i++){
    //printf("%d ", kv_A(watchedClause->lits[0],i));
    //}
    //printf("\n");
    
    foundForReselection=false;

    if(!foundForReselection){
      sizeOfClause = watchedClause->size;
      for(i=0;i<sizeOfClause;i++){
	toReselect = watchedClause->lits[i];
	//printf("toReselect is %d\n",toReselect);
	if( (toReselect!=-l && toReselect!=otherLit) && 
	    model_is_true_or_undef(toReselect,&ad->model)){ //found to reselect
	  foundForReselection =true;
	  break;
	}
      }
    }
  
    //    if( i < sizeOfClause){  //Found lit to reselect
    if( foundForReselection ){  //Found lit to reselect
      dassert(model_is_false(-l,&ad->model));
      dassert(model_is_true_or_undef(toReselect,&ad->model));
      dassert(!model_is_false(toReselect, &ad->model));
      //watchedClause->cachedLit = -l;

      if ( first ) {
	watchedClause->lits[i] = watchedClause->lits[0];
	watchedClause->lits[0] = toReselect;
	//what stores this direction, is overwritten 
	//	ptrToWatchedClauseAddr = (ThNClause**)&(watchedClause->nextWatched1); 
	*ptrToWatchedClauseAddr = watchedClause->nextWatched1;
	//printf("nextWatched1 is %d\n",watchedClause->nextWatched1);
	watchedClause->nextWatched1 = (void*)kv_A(ad->watches,lit_as_uint(toReselect));
      } else {
	watchedClause->lits[i] = watchedClause->lits[1];
	watchedClause->lits[1] = toReselect;
	//what stores this direction, is overwritten 
	//ptrToWatchedClauseAddr = (ThNClause**)&(watchedClause->nextWatched2); 
	*ptrToWatchedClauseAddr = watchedClause->nextWatched2;
	//printf("nextWatched2 is %d\n",watchedClause->nextWatched2);
	watchedClause->nextWatched2 = (void*)kv_A(ad->watches,lit_as_uint(toReselect));
      }
      //update previouslyWatchedClause
      kv_A(ad->watches, lit_as_uint(toReselect)) = watchedClause;
      watchedClause = *ptrToWatchedClauseAddr;
      //__builtin_prefetch (watchedClause->lits->a, 0, 0);
      //printf("Watched clause is %d\n",watchedClause);
      continue;
    }else{//Propagate otherLit or conflict
      //      printf("not found to reselect\n");
      //      exit(-1);
      dassert(i==sizeOfClause);
      if (model_is_undef(otherLit,&ad->model)){ //Propagate
	//printf("Propagating %d with an nclause\n",otherLit);
	//printf("Lit being propagated is %d\n",-l);
	//printf("Watched clause is %d\n",watchedClause);
	r.size     = sizeOfClause;
	r.thNClPtr = watchedClause;
	model_set_true_w_reason(otherLit,r,&ad->model);
	ad->stats.numProps++;
	//dassert(model_get_reason_of_lit(otherLit,&ad->model).thNClPtr == watchedClause);
	//printf("watchedClause is: ",sizeOfClause);
	//for(i=0;i<sizeOfClause;i++){
	//printf("%d ", kv_A(model_get_reason_of_lit(otherLit,&ad->model).thNClPtr->lits[0],i));
	//}
	//printf("\n");
	//	exit(-1);
	if(ad->model.decision_lvl == 0){
	  ad->stats.numDLZeroLits++;
	  ad->stats.numDlZeroLitsSinceLastRestart++;
	}
	//update watchedClause and pointer to it.
	if(first){
	  ptrToWatchedClauseAddr = (ThNClause**)&(watchedClause->nextWatched1);
	}else{
	  ptrToWatchedClauseAddr = (ThNClause**)&(watchedClause->nextWatched2);
	}
	watchedClause = *ptrToWatchedClauseAddr;
	//__builtin_prefetch (watchedClause->lits->a, 0, 0);
	continue;
      }else if(model_is_false(otherLit,&ad->model)){ //Conflict
	//Remember to update activity
	watchedClause->activity++;
	ad->conflict.size = sizeOfClause;
	for(i=0;i<sizeOfClause;i++){
	  kv_A(ad->conflict.lits,i) = watchedClause->lits[i];
	}
	return false;
      }    
    }
    exit(-1);  
  }
  return true;
}

ThNClause* azuDICI_insert_th_clause( AzuDICI* ad, Clause cl, bool isOriginal ){

  ThNClause* newThClause = (ThNClause*)malloc(sizeof(ThNClause)+(cl.size-3)*sizeof(Literal));//3 literals already considered in the size of ThNClause
  //ThNClause threadClause;
  Literal l1,l2;
  l1 = kv_A(cl.lits, 0);
  l2 = kv_A(cl.lits, 1);

  newThClause->activity   = 0;
  newThClause->size       = cl.size;
  newThClause->isOriginal = isOriginal;

  //init nextWatches with appropiate information
  newThClause->nextWatched1  = (void*)kv_A(ad->watches, lit_as_uint(l1)); 
  newThClause->nextWatched2  = (void*)kv_A(ad->watches, lit_as_uint(l2));

  for(int i=0;i<cl.size;i++){
    newThClause->lits[i] =  kv_A(cl.lits, i);
  }

  /*printf("inserting nclause in local db: ");
  for(int i=0;i<kv_size(threadClause.lits[0]);i++){
    printf("%d ",kv_A(threadClause.lits[0],i));
  }
  printf("\n");*/
  //  exit(-1);

  kv_push(ThNClause*, ad->thcdb, newThClause);
  dassert(kv_size(ad->thcdb)>0);
  //insert at beginning of watches linked lists.
  //ThNClause* ptrToThClause = &kv_A(ad->thcdb, kv_size(ad->thcdb)-1);
  kv_A(ad->watches, lit_as_uint(l1)) = newThClause;
  kv_A(ad->watches, lit_as_uint(l2)) = newThClause;
  ad->numNClauses++;
  ad->numClauses++;
  return newThClause;
}

//insertion sort according to height in the model.
void  azuDICI_sort_lits_according_to_DL_from_index(Model m, Clause cl, unsigned int indexFrom){
  int i,j;
  Literal aux;
  for(i=indexFrom;i<cl.size-1;i++){
    for(j=i+1;j<cl.size;j++){
      if( model_get_lit_height(kv_A(cl.lits,j),&m) > 
	  model_get_lit_height(kv_A(cl.lits,i),&m) ){
	aux = kv_A(cl.lits, i);
	kv_A(cl.lits, i) = kv_A(cl.lits, j);
	kv_A(cl.lits, j) = aux;
      }
    }
  }
}

void azuDICI_increase_activity(Reason r){
  if(r.size > 3){
    //printf("About to update score of an nclause with size %d\n",r.size);
    dassert(r.binLit==0);
    dassert(r.tClPtr==NULL);
    ThNClause *nclause;
    nclause = r.thNClPtr;
    dassert(r.thNClPtr);
    //printf("Nclause got\n");
    dassert(nclause);
    //printf("Clause activity is %d \n",nclause->activity);
    nclause->activity++;
    //printf("Score updated\n");
    //    exit(-1);
  }
}

void azuDICI_get_clause_from_reason(Clause *cl, Reason r, Literal l){
  //dassert(r.size>1);
  //printf("Reason has size %d \n",r.size);
  switch(r.size){
  case 1:
    cl->size = 1;
    kv_A(cl->lits,0) = l;
    break;
  case 2:
    cl->size = 2;
    kv_A(cl->lits,0) = l;
    kv_A(cl->lits,1) = r.binLit;
    //printf("other lit is %d\n",r.binLit);
    break;
  default:
    //printf("Extracting n reason\n");
    ;ThNClause *nclause;
    nclause = r.thNClPtr;
    cl->size = r.size;
    //printf("Clause size is %d\n",cl->size);
    //printf("Clause is %d\n",nclause);
    dassert(nclause->lits);
    dassert(nclause->size == r.size);
    for(int i=0; i<r.size;i++){
      //printf("Extracting lit %d\n",kv_A(nclause->lits[0],i));
      kv_A(cl->lits,i) = nclause->lits[i];
    }
  }
}

void azuDICI_notify_unassigned_lit(AzuDICI *ad, Literal l){
  if(ad->strat.decideOverLits)
    maxHeap_insert_element(&ad->heap, lit_as_uint(l));
  else
    maxHeap_insert_element(&ad->heap, var(l));
}

void  azuDICI_increaseScore(AzuDICI* ad, Literal l){
  bool normalized;
  if(ad->strat.decideOverLits)
    normalized = maxHeap_increase_score_in(&ad->heap, lit_as_uint(l), ad->scoreBonus);
  else
    normalized = maxHeap_increase_score_in(&ad->heap, var(l), ad->scoreBonus);

  //if scoreBonus is too high, values in the heap are normalized 
  //we reset scoreBonus
  if(normalized) ad->scoreBonus = ad->strat.initialScoreBonus;
}

void azuDICI_init_thcdb(AzuDICI* ad){
  unsigned int sizeNDB;
  sizeNDB = kv_size(ad->cdb->nDB);
  NClause *nclause;
  ThNClause *localClause;
  Literal l1, l2;

  //UNITARY DB INITIALIZATION
  kv_init(ad->thudb);
  for(int i=0; i<kv_size(ad->cdb->uDB);i++){
    l1 = kv_A(ad->cdb->uDB,i);
    kv_push(Literal, ad->thudb, l1);
  }

  ad->lastUnitAdded = ad->cdb->numOriginalUnits;
  dassert(ad->lastUnitAdded == ad->cdb->numUnits);
  ad->numUnits = ad->cdb->numUnits;

  //BINARY DB INITIALIZATION
  kv_init(ad->thbdb);//init vector 
  kv_resize(kvec_t(Literal), ad->thbdb, 2*(ad->nVars+1) ); //We know size is fixed to 2*(nVars+1) elements
  kv_size(ad->thbdb)= 2*(ad->nVars+1);

  for(int i=0;i<2*(ad->nVars+1);i++){
    kv_init(kv_A(ad->thbdb,i));
    //    kv_resize(Literal, kv_A(cdb->bDB,i), MIN_MEM_LIT);
  }

  for(int i=0;i<2*(ad->nVars+1);i++){
    for(int j=0;j<kv_size(kv_A(ad->cdb->bDB,i));j++){
      l1 = kv_A(kv_A(ad->cdb->bDB,i),j);
      kv_push(Literal, kv_A(ad->thbdb,i), l1);
    }
  }

  ad->numBinaries = ad->cdb->numBinaries;

  //NCLAUSEDB Initialization
  //localClause.activity = 0;
  for(int i=0; i<sizeNDB; i++){
    //    ts_vec_ith(nclause, ad->cdb->nDB, i);
    nclause = &kv_A(ad->cdb->nDB, i);
    unsigned int sizeOfClause=nclause->lits[0]; 
    localClause = (ThNClause*)malloc(sizeof(ThNClause)+(sizeOfClause-3)*sizeof(Literal));//3 Literals already considered in the size of the ThNClause

    //remember lits[0] is size
    l1 = nclause->lits[1];
    l2 = nclause->lits[2];
    //    printf("In new n clause watching %d, %d \n", l1, l2);
    localClause->activity     = 0;
    localClause->size         = sizeOfClause;
    localClause->isOriginal   = true;
    localClause->nextWatched1 = (void*)kv_A(ad->watches, lit_as_uint(l1));
    localClause->nextWatched2 = (void*)kv_A(ad->watches, lit_as_uint(l2));
    for(int j=0;j<sizeOfClause;j++){
      localClause->lits[j] = nclause->lits[j+1];
    }
    //    dassert(kv_size(localClause->);
    //    printf("New inserted n clause with size %d\n", kv_size(nclause->lits));
    //    printf("New inserted n clause with size %d\n", kv_size(*(localClause.lits)));
    //    localClause.posInDB = i;

    kv_push(ThNClause*, ad->thcdb, localClause);
    kv_A(ad->watches, lit_as_uint(l1)) = localClause;
    kv_A(ad->watches, lit_as_uint(l2)) = localClause;
  }

  ad->lastNaryAdded = ad->cdb->numOriginalNClauses;
  dassert(ad->lastNaryAdded == ad->cdb->numNClauses);
  ad->numNClauses = ad->cdb->numNClauses;
  ad->numClauses = ad->cdb->numClauses;
}

bool azuDICI_set_true_units(AzuDICI *ad){
  //  unsigned int numUnits;
  //  numUnits = ad->cdb->numOriginalUnits;
  //numUnits = ad->cdb->numUnits;
  Literal unitToSetTrue;
  Reason r;

  r.size     = 1;
  r.binLit   = 0;
  r.tClPtr   = NULL;
  r.thNClPtr = NULL;

  dassert(ad->model.decision_lvl==0);
  for(int i=0;i<ad->numUnits;i++){
    unitToSetTrue = kv_A(ad->thudb,i);
    if (model_is_undef(unitToSetTrue,&ad->model)){
      //r.binLit = -unitToSetTrue;
      model_set_true_w_reason(unitToSetTrue,r,&ad->model);
      ad->stats.numDLZeroLits++;
      ad->stats.numDlZeroLitsSinceLastRestart++;      
    }else if(model_is_false(unitToSetTrue,&ad->model)){
      return false;
    }    
  }
  return true;
}

void azuDICI_print_clause(AzuDICI* ad, Clause cl){
  Literal l;
  for(int i=0;i<cl.size;i++){
    l = kv_A(cl.lits,i);
    if(model_is_undef(l, &ad->model))  printf("U[]%d ",l);
    else if(model_is_true(l, &ad->model)) 
      printf("T[dl:%d] %d ",model_get_lit_dl(l, &ad->model),l);
    else 
      printf("F[dl:%d] %d ",model_get_lit_dl(l, &ad->model),l);
  }
  printf("\n");
}

/*******************Make this Thread safe****************************/
  /*bDB stores 2 literals in a clause as following:
   dDB is a vector which size is 2*(numVars+1) and each position in the vector
   corresponds to a literal which has a literal vector associated to it. If the 
   clause (x1 or ~x2) is to be stored in dBD, we store x1 in the literal vector
   associated to the literal x2, and we store ~x2 in the literal vector associated
   to literal ~x1. This way we have an implication vector for all literals. 
   Associating x1 with x2 means that if x2 were to be true, then x1 must also be true.
   Associating ~x2 with ~x1 means that if ~x1 were to be true, then ~x2 must also be true*/
void azuDICI_insert_binary_clause(AzuDICI* ad, Clause *cl){
  dassert(cl->size == 2);
  /*We are storing implications, so we want to know the negation of each literal
   that belongs to the binary clause*/
  Literal l1,l2;
  l1 = kv_A(cl->lits,0);
  l2 = kv_A(cl->lits,1);

  unsigned int not_l1,not_l2;
  not_l1 = lit_as_uint(-l1);
  not_l2 = lit_as_uint(-l2);

  kv_push(Literal,kv_A(ad->thbdb, not_l1),l2);
  //If it is not in list, it's not in the other list either
  kv_push(Literal, kv_A(ad->thbdb, not_l2),l1);
  ad->numBinaries++;
  ad->numClauses++;
}

 /*Inserting, in the clause database, an input clause that only has one literal*/
void azuDICI_insert_unitary_clause(AzuDICI* ad, Clause *cl){
  dassert(cl->size == 1);
  dassert(listSize == cdb->numUnits);

  kv_push( Literal, ad->thudb, kv_A(cl->lits,0) );//Here vector can be rellocated
  ad->numUnits++;
  ad->numClauses++;
}

#endif
