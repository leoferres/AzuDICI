#ifndef _AZUDICI_C_
#define _AZUDICI_C_

#include "azudici.h"

AzuDICI* azuDICI_init(ClauseDB* generalClauseDB, unsigned int wId){
  int i;

  AzuDICI *ad = (AzuDICI*)malloc(sizeof(AzuDICI));
  unsigned int  dbSize;

  strategy_init(&(ad->strat), wId);  //strategy init according to wId

  ad->wId                 = wId;
  ad->cdb                 = generalClauseDB;
  ts_vec_size( dbSize, ad->cdb->uDB );
  ad->lastUnitAdded       = dbSize;
  ts_vec_size( dbSize, ad->cdb->nDB );
  ad->lastNaryAdded       = dbSize;
  ad->randomNumberIndex   = 0;
  ad->dlToBackjump        = 0;
  ad->dlToBackjumpPos     = 0;
  ad->scoreBonus          = ad->strat.initialScoreBonus;
  ad->currentRestartLimit = ad->strat.initialRestartLimit;
  ad->currentCleanupLimit = ad->strat.initialCleanupLimit;

  //lastBinariesAdded
  kv_init( ad->lastBinariesAdded );
  kv_resize(unsigned int,  ad->lastBinariesAdded, 2*(ad->cdb->numVars+1) );
  kv_size(ad->lastBinariesAdded) = 2*(ad->cdb->numVars+1);
  int listSize;
  for( i=0; i < 2*(ad->cdb->numVars + 1) ; i++ ){
    ts_vec_size( listSize, kv_A(ad->cdb->bDB, i) );
    kv_A(ad->lastBinariesAdded, i) = listSize;
  }

  //varMarks
  kv_init(ad->varMarks);
  kv_resize(bool, ad->varMarks, ad->cdb->numVars+1);
  kv_size(ad->varMarks) = ad->cdb->numVars+1;
  for( i=0; i < ad->cdb->numVars + 1 ; i++ ){
    kv_A(ad->varMarks, i) = false;
  }

  //thcdb (thread clause data base)
  kv_init(ad->thcdb);
  kv_resize(ThNClause*, ad->thcdb, 30000000); //no more than 10000000 nclauses

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
  printf("Solve called\n");
  if( !azuDICI_set_true_units(ad) ) return 20;
  printf("Units set true\n");
  Literal dec;
  Reason r;
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
    }
    azuDICI_restart_if_adequate(ad);
    if( !azuDICI_clause_cleanup_if_adequate(ad) ) return 20;
    
    //printf("Decide\n");
    dec = azuDICI_decide(ad);
    if (dec == 0){
      for(int i=1; i<= ad->cdb->numVars; i++){
	dassert(!model_is_undef(i,&ad->model));
      }
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
  //printf("Propagate called\n");
  lit=1;
  while ( lit != 0 ) {
    lit = model_next_lit_for_2_prop( &ad->model);
    //printf("2 prop with lit %d, from %d\n",lit,ad->cdb->numVars);
    if ( lit!= 0 ) {  
      if ( !azuDICI_propagate_w_binaries(ad, lit)) return false;
      else continue; 
    } 
    
    lit = model_next_lit_for_3_prop( &ad->model);
    //printf("3 prop with lit %d\n",lit);
    if ( lit != 0 ) {  
      if( !azuDICI_propagate_w_ternaries(ad, lit)) return false;
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
    azuDICI_get_clause_from_reason(ad, &cl, r,poppedLit);
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
  kv_resize(Literal, cl.lits, 2*ad->cdb->numVars);
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
      //      r = model.reasonOfLit(litOfLemma.negation());
      r = model_get_reason_of_lit(litOfLemma,&ad->model);
      //printf("getting reason of litOfLemma %d\n",litOfLemma);
      azuDICI_get_clause_from_reason(ad, &cl, r,litOfLemma);
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
      
	r = model_get_reason_of_lit(-testingLit,&ad->model);
	azuDICI_get_clause_from_reason(ad, &cl, r,-testingLit);
	//printf("Clause reason is ");azuDICI_print_clause(ad,cl);
      
	//cl = clauseReasonOfLit(testingLit.negation());
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
      if(testingIndex==lastMarkedInLemma) dassert(litIsRedundant);
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
  Literal l1,l2,l3;
  //printf("Learning lemma size %d: ",ad->lemmaToLearn.size); azuDICI_print_clause(ad,ad->lemmaToLearn);
  switch(ad->lemmaToLearn.size){
  case 1:
    insert_unitary_clause(ad->cdb, &ad->lemmaToLearn, false);
    ad->rUIP.size     = 1;
    ad->rUIP.binLit   = 0;
    ad->rUIP.tClPtr   = NULL;
    ad->rUIP.thNClPtr = NULL;
    dassert(ad->dlToBackjump==0);
    break;

  case 2:
    l1 = kv_A(ad->lemmaToLearn.lits,0);
    l2 = kv_A(ad->lemmaToLearn.lits,1);
    kv_A(ad->lastBinariesAdded,lit_as_uint(-l1))++; //hack for same search
    kv_A(ad->lastBinariesAdded,lit_as_uint(-l2))++; //hack for same search

    insert_binary_clause(ad->cdb, &ad->lemmaToLearn, false);
    ad->rUIP.size     = 2;
    ad->rUIP.binLit   = kv_A(ad->lemmaToLearn.lits,1);
    ad->rUIP.tClPtr   = NULL;
    ad->rUIP.thNClPtr = NULL;
    break;

  case 3:
    //printf("learning 3 clause\n");
    l1 = kv_A(ad->lemmaToLearn.lits,0);
    l2 = kv_A(ad->lemmaToLearn.lits,1);
    l3 = kv_A(ad->lemmaToLearn.lits,2);

    Literal *ptr = insert_ternary_clause(ad->cdb, &ad->lemmaToLearn, false,ad->wId);
    //printf("Inserted\n");
    ad->rUIP.size     = 3;
    ad->rUIP.binLit   = 0;
    ad->rUIP.tClPtr   = ptr;
    ad->rUIP.thNClPtr = NULL;

    dassert(model_is_undef(l1,&ad->model));
    dassert(model_get_lit_dl(l2,&ad->model)>=model_get_lit_dl(l3,&ad->model));
    dassert(model_get_lit_height(l2,&ad->model)>model_get_lit_height(l3,&ad->model));

    kv_A(ad->lemmaToLearn.lits,0) = l1;
    kv_A(ad->lemmaToLearn.lits,1) = l2;
    kv_A(ad->lemmaToLearn.lits,2) = l3;
    break;

  default:
    //printf("learning n clause\n");
    ;Literal uip    = kv_A(ad->lemmaToLearn.lits,0);
    Literal hdlLit = kv_A(ad->lemmaToLearn.lits,1);

    dassert(ad->lemmaToLearn.size>3);
    //here, lemmaToLearn will be sorted
    unsigned int indexInDB = 
      insert_nary_clause(ad->cdb, &ad->lemmaToLearn, false, ad->wId);

    /*put in lemmaToLearn[0] the uip */
    //highest decision level is ad->dlToBackjump   
    Literal aux;
    for(int i=1; i < ad->lemmaToLearn.size; i++){
      if( kv_A(ad->lemmaToLearn.lits,i) == uip ){
	kv_A(ad->lemmaToLearn.lits,0) = uip;
	kv_A(ad->lemmaToLearn.lits,i) = kv_A(ad->lemmaToLearn.lits,0);
	break;
      }
    }

    /*put in lemmaToLearn[1] literal with highest DL*/
    //highest decision level is ad->dlToBackjump   
    //Literal aux;
    for(int i=1; i < ad->lemmaToLearn.size; i++){
      if( kv_A(ad->lemmaToLearn.lits,i) == hdlLit ){
	dassert(model_get_lit_dl(kv_A(ad->lemmaToLearn.lits,i),&ad->model) == ad->dlToBackjump);
	kv_A(ad->lemmaToLearn.lits,i) = kv_A(ad->lemmaToLearn.lits,1);
	kv_A(ad->lemmaToLearn.lits,1) = hdlLit;
	break;
      }
    }

    ThNClause *ptr2 = 
      azuDICI_insert_th_clause(ad, ad->lemmaToLearn, false, indexInDB);

    ad->rUIP.size     = ad->lemmaToLearn.size;
    ad->rUIP.binLit   = 0;
    ad->rUIP.tClPtr   = NULL;
    ad->rUIP.thNClPtr = ptr2;
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
    ad->stats.numCleanups                  = 0;
    ad->currentCleanupLimit *= ad->strat.cleanupMultiplier;

    printf("CLEANUP\n");

    //Cleanup
    unsigned int i,j, nonFalseLits;
    Clause tentativeTernaryOrBinary;
    kv_init(tentativeTernaryOrBinary.lits);
    kv_resize(Literal, tentativeTernaryOrBinary.lits, ad->cdb->numVars);
    unsigned int keptLiterals;
    ThNClause cl;
    NClause *generalCl;
    bool delete;
    Reason r;
    r.size     = 1;
    r.binLit   = 0;
    r.tClPtr   = NULL;
    r.thNClPtr = NULL;

    //azucidi_cleanup_watches(ad);
    for( i=0; i < 2*(ad->cdb->numVars + 1) ; i++ ){
      kv_A(ad->watches, i) = NULL;
    }

    for(i=0;i<kv_size(ad->thcdb);i++){
      cl = kv_A(ad->thcdb, i);
      delete = true;
      ts_vec_ith_ma(generalCl, ad->cdb->nDB, cl.posInDB);
      if( generalCl->is_original ||
 	  cl.activity >= ad->strat.activityThreshold ){ //keep it
	delete = false;
 	tentativeTernaryOrBinary.size=0;
 	keptLiterals = 0;
 	for(j=0; j<kv_size(*cl.lits); j++){
 	  if(model_is_true(kv_A(*(cl.lits),j), &ad->model)){
	    numDeletes++;
	    numTrueClauses++;
 	    delete = true; //it is true at dl 0, no need to have it.
 	    break;
 	  }else if(model_is_undef(kv_A(*(cl.lits),j), &ad->model)){
 	    kv_A(tentativeTernaryOrBinary.lits,tentativeTernaryOrBinary.size++) = kv_A(*(cl.lits),j);	    
 	  }
 	}

 	if( !delete && tentativeTernaryOrBinary.size <= 3 ){ //new unit, binary or ternary
	  numDeletes++;
 	  delete = true;
	  if(tentativeTernaryOrBinary.size == 1 ){
	    numUnits++;
	    insert_unitary_clause(ad->cdb, &tentativeTernaryOrBinary, generalCl->is_original);
	    if (model_is_undef(kv_A(tentativeTernaryOrBinary.lits,0), &ad->model)){
	      model_set_true_w_reason(kv_A(tentativeTernaryOrBinary.lits,0),r,&ad->model);
	      ad->stats.numDLZeroLits++;
	      ad->stats.numDlZeroLitsSinceLastRestart++;      
	    }else if(model_is_false(kv_A(tentativeTernaryOrBinary.lits,0),&ad->model)){
	      return false;
	    }
	  }else if(tentativeTernaryOrBinary.size == 2){
	    numBinaries++;
	    insert_binary_clause(ad->cdb, &tentativeTernaryOrBinary, generalCl->is_original);
 	    //PairOfIndex = insert_binary_clause();
 	    //if(lastBinariesAdded[lit_as_uint(tentativeTernaryOrBinary.lits[0])]==pair.first-1 &&  pair.second){
	    //lastBinariesAdded[lit_as_uint(tentativeTernaryOrBinary.lits[0])]++;
	    //lastBinariesAdded[lit_as_uint(tentativeTernaryOrBinary.lits[0])]++;
 	  }else{
	    numTernaries++;
	    dassert(tentativeTernaryOrBinary.size == 3);
 	    insert_ternary_clause(ad->cdb, &tentativeTernaryOrBinary, generalCl->is_original, ad->wId); //clause will be activated with wId
 	  }
 	}
      }else{
	numDeletes++;
	numRealDeletes++;
      }

      if(delete){
 	generalCl->flags[ad->wId]=false; //logical delete
	cl.posInDB = -1; //that's how we will know if is deleted
      }
    }

    azuDICI_compact_and_watch_thdb(ad);
    kv_destroy(tentativeTernaryOrBinary.lits);
    printf("numUnits = %d\n",numUnits);
    printf("numBinaries = %d\n",numBinaries);
    printf("numTernaries = %d\n",numTernaries);
    printf("numTrueClauses = %d\n",numTrueClauses);
    printf("numDeletes = %d\n",numDeletes);
    printf("numRealDeletes = %d\n",numRealDeletes);

    return azuDICI_propagate(ad);
  }
  return true;
}

void azuDICI_compact_and_watch_thdb(AzuDICI* ad){
  uint lastValidPos=0;
  Literal l1, l2;
  ThNClause cl;
  for(int i=0;i<kv_size(ad->thcdb);i++){
    cl = kv_A(ad->thcdb, i);
    if(cl.posInDB!=-1){//If kept, normalize activities
      cl.activity = cl.activity/2;
      //watch same two literals that were being watched
      l1 = cl.lwatch1;
      l2 = cl.lwatch2;
      //pointer to general db keeps being valid. so lits and posInDB, don't change
      cl.nextWatched1  = (void*)kv_A(ad->watches, lit_as_uint(l1)); 
      cl.nextWatched2  = (void*)kv_A(ad->watches, lit_as_uint(l2));

      kv_A(ad->thcdb,lastValidPos++) = cl; //copy it to last valid pos in thcdb

      ThNClause* ptrToThClause = &kv_A(ad->thcdb, lastValidPos-1);
      kv_A(ad->watches, lit_as_uint(l1)) = ptrToThClause;
      kv_A(ad->watches, lit_as_uint(l2)) = ptrToThClause;
    }
  }
  kv_size(ad->thcdb) = lastValidPos;
}

void azuDICI_restart_if_adequate(AzuDICI* ad){
  if( ad->stats.numConflictsSinceLastRestart >= ad->currentRestartLimit ){
    printf("Restart\n");
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
    v = (unsigned int) ( (double)rNumber/RAND_MAX * ad->cdb->numVars ) + 1;
    for( i=1;i<1000;i++ ) {
      //printf("v is %d\n",v);
      if (model_is_undef_var(v,&ad->model)) break;
      v = (v+1) % ad->cdb->numVars +1;
    }

    //printf("random decision is %d and i is %d\n",v,i);
    if ( !model_is_undef_var(v,&ad->model) ) v=0;
  }

  //printf("random decision is %d\n",v);

  if (ad->strat.decideOverLits){
    if(v!=0){
      unsigned int pol;
      pol= ad->cdb->randomNumbers[ad->randomNumberIndex++];
      if( pol > (RAND_MAX/2) ) l=v;
      else l = -v;
    }
  }


  if(ad->strat.decideOverLits){
    l=0;
    if (!v) 
      do {
	l = uint_as_lit(maxHeap_remove_max(&ad->heap));
	//printf("heap decision is %d\n",l);
	if (!l) return(0);
      } while (!model_is_undef_var(var(l),&ad->model));
    dassert( var(l) <= ad->cdb->numVars );
    return l;  
  }



  //Decide over vars depends on other strat parameters.  
  if (!v) do {
      v = maxHeap_remove_max(&ad->heap);
      //printf("heap decision is %d\n",v);
      if (!v) return(0);
    } while (!model_is_undef_var(v,&ad->model));


  dassert(model_is_undef_var(v,&ad->model)); 
  dassert(v <= ad->cdb->numVars );
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
  sizeOfList = kv_A(ad->lastBinariesAdded,litIndex); //hack for same search
  //  ts_vec_size(sizeOfList, list);
  Reason r;
  r.tClPtr = NULL;
  r.thNClPtr = NULL;

  for(int i=0;i<sizeOfList;i++){
    ts_vec_ith(lToSetTrue, kv_A(ad->cdb->bDB,litIndex), i);
    if (model_is_undef(lToSetTrue,&ad->model)){
      r.size   = 2;
      r.binLit = -l;
      model_set_true_w_reason(lToSetTrue,r,&ad->model);
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

bool azuDICI_propagate_w_ternaries(AzuDICI* ad, Literal l){

  //Propagate if proper flag is set to true
  //  printf("3 propagating lit %d\n",l);
  unsigned int indexInTDB;
  Literal lToSetTrue;
  unsigned int sizeOfList;
  unsigned int litIndex = lit_as_uint(-l);
  ts_vec_size(sizeOfList, kv_A(ad->cdb->ternaryWatches, litIndex));
  Reason r;
  r.size = 3;
  r.binLit = 0;
  r.thNClPtr = NULL;
  TClause *ternaryClause;
  Literal l1,l2,l3;
  for(int i=0;i<sizeOfList;i++){
    ts_vec_ith(indexInTDB, kv_A(ad->cdb->ternaryWatches, litIndex), i);
    ts_vec_ith_ma(ternaryClause,ad->cdb->tDB,indexInTDB);
    if(ternaryClause->flags[ad->wId]){ //Only propagate with self-learned clauses
      l1 = ternaryClause->lits[0];
      l2 = ternaryClause->lits[1];
      l3 = ternaryClause->lits[2];
      //  printf("lit %d participates in 3 clause %d v %d v %d \n",-l, l1, l2, l3);
      lToSetTrue = 0;
      dassert(l1==-l || l2==-l ||l3==-l);
      if(model_is_false(l1,&ad->model) && model_is_false(l2,&ad->model)){
	lToSetTrue = l3;
      }else if (model_is_false(l2,&ad->model) && model_is_false(l3,&ad->model)){
	lToSetTrue = l1;
      }else if (model_is_false(l1,&ad->model) && model_is_false(l3,&ad->model)){
	lToSetTrue = l2;
      }else{
	continue;
      }

      dassert(lToSetTrue!=0);
      if (model_is_undef(lToSetTrue,&ad->model)){
        //TClause *tmpPtr;
        //ts_vec_ith_ma(tmpPtr,ad->cdb->tDB,indexInTDB);
	r.tClPtr = ternaryClause;//&tmpPtr->lits[0]; //check if this works
	model_set_true_w_reason(lToSetTrue,r,&ad->model);
	if(ad->model.decision_lvl ==0){
	  ad->stats.numDLZeroLits++;
	  ad->stats.numDlZeroLitsSinceLastRestart++;
	}
      }else if(model_is_false(lToSetTrue,&ad->model)){ //Conflict
	ad->conflict.size = 3;
	kv_A(ad->conflict.lits,0) = l1;
	kv_A(ad->conflict.lits,1) = l2;
	kv_A(ad->conflict.lits,2) = l3;
	//azuDICI_print_clause(ad, ad->conflict);
	return false;
      }    
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

    if (watchedClause->lwatch1 == -l){
      //printf("Is first watch %d\n",watchedClause->lwatch1);
      first=true;
      otherLit = watchedClause->lwatch2;
    } else{
      //printf("Is second watch  %d\n", watchedClause->lwatch2);
      dassert(watchedClause->lwatch2 == -l);
      first=false;
      otherLit = watchedClause->lwatch1;
    }

    if(model_is_true(otherLit,&ad->model)){
      //Update watchedLiteral and previouslyWatchedLiteral
      if(first){
	ptrToWatchedClauseAddr =(ThNClause**)&(watchedClause->nextWatched1);
	//watchedClause = watchedClause->nextWatched1; see below, is the same
      }else{
	ptrToWatchedClauseAddr =(ThNClause**)&(watchedClause->nextWatched2);
	//watchedClause = watchedClause->nextWatched2; see below, is the same
      }
      watchedClause = *ptrToWatchedClauseAddr;
      //printf("Watched clause is %d\n",watchedClause);
      continue;
    }

    //if other watched is not true, try to reselect.
    sizeOfClause = kv_size(watchedClause->lits[0]);
    //printf("with clause with size %d: ",sizeOfClause);
    //for(i=0;i<sizeOfClause;i++){
    //printf("%d ", kv_A(watchedClause->lits[0],i));
    //}
    //printf("\n");
    
    foundForReselection=false;
    for(i=0;i<sizeOfClause;i++){
      toReselect = kv_A(watchedClause->lits[0], i);
      //printf("toReselect is %d\n",toReselect);
      if( (toReselect!=-l && toReselect!=otherLit) && 
	  model_is_true_or_undef(toReselect,&ad->model)){ //found to reselect
	dassert(!model_is_false(toReselect, &ad->model));
	foundForReselection =true;
	break;
      }
    }
  
    //    if( i < sizeOfClause){  //Found lit to reselect
    if( foundForReselection ){  //Found lit to reselect
      dassert(model_is_false(-l,&ad->model));
      dassert(model_is_true_or_undef(toReselect,&ad->model));
      if ( first ) {
	watchedClause->lwatch1 = toReselect;
	//what stores this direction, is overwritten 
	//	ptrToWatchedClauseAddr = (ThNClause**)&(watchedClause->nextWatched1); 
	*ptrToWatchedClauseAddr = watchedClause->nextWatched1;
	//printf("nextWatched1 is %d\n",watchedClause->nextWatched1);
	watchedClause->nextWatched1 = (void*)kv_A(ad->watches,lit_as_uint(toReselect));
      } else {
	watchedClause->lwatch2 = toReselect;
	//what stores this direction, is overwritten 
	//ptrToWatchedClauseAddr = (ThNClause**)&(watchedClause->nextWatched2); 
	*ptrToWatchedClauseAddr = watchedClause->nextWatched2;
	//printf("nextWatched2 is %d\n",watchedClause->nextWatched2);
	watchedClause->nextWatched2 = (void*)kv_A(ad->watches,lit_as_uint(toReselect));
      }
      //update previouslyWatchedClause
      kv_A(ad->watches, lit_as_uint(toReselect)) = watchedClause;
      watchedClause = *ptrToWatchedClauseAddr;
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
	continue;
      }else if(model_is_false(otherLit,&ad->model)){ //Conflict
	//Remember to update activity
	watchedClause->activity++;
	ad->conflict.size = sizeOfClause;
	for(i=0;i<sizeOfClause;i++){
	  kv_A(ad->conflict.lits,i) = kv_A(watchedClause->lits[0],i);
	}
	return false;
      }    
    }
    exit(-1);  
  }
  return true;
}

ThNClause* azuDICI_insert_th_clause(AzuDICI* ad, Clause cl, bool isOriginal, unsigned int indexInDB){
  ThNClause threadClause;
  Literal l1,l2;
  l1 = kv_A(cl.lits, 0);
  l2 = kv_A(cl.lits, 1);

  threadClause.activity  = 0;
  threadClause.lwatch1   = l1;
  //if cl is learned lemma, ensure this is the highes dl literal
  threadClause.lwatch2   = l2;

  //init nextWatches with appropiate information
  threadClause.nextWatched1  = (void*)kv_A(ad->watches, lit_as_uint(l1)); 
  threadClause.nextWatched2  = (void*)kv_A(ad->watches, lit_as_uint(l2));

  NClause *clInDB;
  ts_vec_ith_ma(clInDB, ad->cdb->nDB, indexInDB);
  threadClause.lits = &clInDB->lits;
  threadClause.posInDB      = indexInDB; //We need this for clause cleanup
  /*printf("inserting nclause in local db: ");
  for(int i=0;i<kv_size(threadClause.lits[0]);i++){
    printf("%d ",kv_A(threadClause.lits[0],i));
  }
  printf("\n");*/
  //  exit(-1);

  kv_push(ThNClause, ad->thcdb, threadClause);
  dassert(kv_size(ad->thcdb)>0);
  //insert at beginning of watches linked lists.
  ThNClause* ptrToThClause = &kv_A(ad->thcdb, kv_size(ad->thcdb)-1);
  kv_A(ad->watches, lit_as_uint(l1)) = ptrToThClause;
  kv_A(ad->watches, lit_as_uint(l2)) = ptrToThClause;

  return ptrToThClause;
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

void azuDICI_get_clause_from_reason(AzuDICI* ad, Clause *cl, Reason r, Literal l){
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
  case 3:
    cl->size = 3;
    TClause *tclause;
    tclause = r.tClPtr;
    //printf("Num of ternary clauses is %d\n",ad->cdb->numTernaries);
    //printf("Ternary pointer got\n");
    //kv_A(cl->lits,0) = l;
    kv_A(cl->lits,0) = tclause->lits[0];
    kv_A(cl->lits,1) = tclause->lits[1];
    kv_A(cl->lits,2) = tclause->lits[2];
    break;
  default:
    //printf("Extracting n reason\n");
    ;ThNClause *nclause;
    nclause = r.thNClPtr;
    cl->size = r.size;
    //printf("Clause size is %d\n",cl->size);
    //printf("Clause is %d\n",nclause);
    dassert(nclause->lits);
    for(int i=0; i<r.size;i++){
      //printf("Extracting lit %d\n",kv_A(nclause->lits[0],i));
      kv_A(cl->lits,i) = kv_A(nclause->lits[0],i);
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
  ts_vec_size(sizeNDB, ad->cdb->nDB);
  NClause *nclause;
  ThNClause localClause;
  Literal l1, l2;

  //localClause.activity = 0;
  for(int i=0; i<sizeNDB; i++){
    //    ts_vec_ith(nclause, ad->cdb->nDB, i);
    ts_vec_ith_ma(nclause, ad->cdb->nDB, i);
    l1 = kv_A(nclause->lits,0);
    l2 = kv_A(nclause->lits,1);
    //    printf("In new n clause watching %d, %d \n", l1, l2);
    localClause.activity = 0;
    localClause.lwatch1 = l1;
    localClause.lwatch2 = l2;
    localClause.nextWatched1 = (void*)kv_A(ad->watches, lit_as_uint(l1));
    localClause.nextWatched2 = (void*)kv_A(ad->watches, lit_as_uint(l2));
    localClause.lits = &(nclause->lits);
    dassert(kv_size(localClause.lits[0]));
    //    printf("New inserted n clause with size %d\n", kv_size(nclause->lits));
    //    printf("New inserted n clause with size %d\n", kv_size(*(localClause.lits)));
    localClause.posInDB = i;

    kv_push(ThNClause, ad->thcdb, localClause);
    int posInVector = kv_size(ad->thcdb)-1;

    kv_A(ad->watches, lit_as_uint(l1)) = &(kv_A(ad->thcdb, posInVector));
    kv_A(ad->watches, lit_as_uint(l2)) = &(kv_A(ad->thcdb, posInVector));
    dassert(&(kv_A(ad->thcdb, posInVector))!=NULL);
    //printf("ThnClause exists\n");
  }
}

bool azuDICI_set_true_units(AzuDICI *ad){
  unsigned int numUnits;
  ts_vec_size(numUnits, ad->cdb->uDB);
  Literal unitToSetTrue;
  Reason r;

  r.size     = 1;
  r.binLit   = 0;
  r.tClPtr   = NULL;
  r.thNClPtr = NULL;

  dassert(ad->model.decision_lvl==0);
  for(int i=0;i<numUnits;i++){
    ts_vec_ith(unitToSetTrue, ad->cdb->uDB,i);

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

#endif
