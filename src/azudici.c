#ifndef _AZUDICI_C_
#define _AZUDICI_C_

#include "clausedb.h"
#include "azudici.h"

AzuDICI* azuDICI_init(ClauseDB* generalClauseDB, unsigned int wId){
  int i;

  AzuDICI *ad = (AzuDICI*)malloc(sizeof(AzuDICI));
  unsigned int  dbSize;
  ad->wId                = wId;
  ad->cdb                = generalClauseDB;
  ts_vec_size( dbSize, db->uDB );
  ad->lastUnitAdded      = dbSize;
  ts_vec_size( dbSize, db->nDB );
  ad->lastNaryAdded      = dbSize;
  ad->randomNumberIndex  = 0;
  ad->dlToBackjump       = 0;
  ad->dlToBackjumpPos    = 0;
  //  ad->scoreBonus         = some_magic_value;  

  //lastBinariesAdded
  kvec_init( ad->lastBinariesAdded );
  kvec_resize(unsigned int,  ad->lastBinariesAdded, 2*(ad->cdb->numVars+1) );
  int listSize;
  for( i=0; i < 2*(ad->cdb->numVars + 1) ; i++ ){
    ts_vec_size( listSize, kv_A(ad->cdb->bDB, i) );
    kv_A(ad->lastBinariesAdded, i) = listSize;
  }

  //varMarks
  kvec_init(ad->varMarks);
  kvec_resize(bool, ad->varMarks, ad->cdb->numVars+1);
  for( i=0; i < ad->cdb->numVars + 1 ; i++ ){
    kv_A(ad->varMarks, i) = false;
  }

  //thcdb (thread clause data base)
  kvec_init(ad->thcdb);

  //watches
  kvec_init(ad->watches);
  kvec_resize(ThNClause*,  ad->watches, 2*(ad->cdb->numVars + 1) );
  for( i=0; i < 2*(ad->cdb->numVars + 1) ; i++ ){
    kv_A(ad->watches, i) = NULL;
  }

  //conflict clause
  kvec_init(conflict.lits);
  kvec_resize(Literal, conflict.lits, ad->db->numVars);
  conflict.size = 0;

  //lemma clause
  kvec_init(lemma.lits);
  kvec_resize(Literal, lemma.lits, ad->db->numVars);
  lemma.size = 0;

  //shortenned lemma clause
  kvec_init(lemmaToLearn.lits);
  kvec_resize(Literal, lemmaToLearn.lits, ad->db->numVars);
  lemmaToLearn.size = 0;

  model_init(ad->model);  //model
  heap_init(ad->heap);    //heap
  stats_init(ad->stats);  //stats
  strat_init(ad->strat);  //strategy Do we initialize this?

  azuDICI_init_thcdb(ad); //here we link and watch to the general cdb->nDB

  return ad;
}

unsigned int azuDICI_solve(AzuDICI* ad){
  //setTrue at dl 0 all unit clauses
  if( !azuDICI_set_true_units(ad) ) return 20;
  Literal dec;
  Reason r;

  while (true) {
    while( !azuDICI_propagate(ad) ){ //While conflict in propagate
      if( ad->model.decisionLevel == 0 ) return 20;
      azuDICI_conflict_analysis(ad);
      azuDICI_lemma_shortening(ad);
      azuDICI_learnLemma(ad);
      azuDICI_backjump_to_dl(ad,ad->dlToBackjump);
      azuDICI_set_true_uip(ad);
    }
    
    azuDICI_restart_if_adequate(ad);
    if( !azuDICI_cleanup_if_adequate(ad) ) return 20;
    
    dec = azuDICI_get_next_decision(ad);
    if (dec == 0)  return 10;
    
    model_set_true__decision( ad->model, dec );
  }
  
  return 0;
}

bool azuDICI_propagate( AzuDICI* ad ){
  Literal lit;  // from highest priority to lowest
  
  lit=1;
  while ( lit != 0 ) {
    lit = model_next_lit_for_2_prop( ad->model);
    if ( lit!= 0 ) {  
      if ( !azuDICI_propagate_w_binaries(ad, lit)) return false;
      else continue; 
    } 
    
    lit = model_next_lit_for_3_prop( ad->model);
    if ( lit != 0 ) {  
      if( !azuDICI_propagate_w_ternaries(ad, lit)) return false;
      else continue; 
    } 
    
    lit = model_next_lit_for_n_prop( ad->model);
    if ( lit != 0 ) {  
      if( !azuDICI_propagate_w_n_clauses(ad, lit)) return false; 
      else continue; 
    } 
  }
  return true;
}


void azuDICI_conflict_analysis(AzuDICI* ad){
  //scoreBonus *= strat.scoreBonusInflationFactor; //Lits activity not yet implemented
  ad->lemma.size = 0;
  
  uint i, numLitsThisDLPending=0;
  Literal lit, poppedLit=0;
  uint index=0;
  Clause cl = ad->conflict;
  Reason r;
  
  while(true){
    //cl->increaseActivity(); This should be incremented when processing the reason or when conflict detected......
    
    while( index < cl.size ){//traverse conflictingclause/currentreasonclause,
      lit = kv_A( cl.lits, index ); 
      index++; // marking literals and counting lits of this DL
      azuDICI_increaseScore(ad, lit);
      if( lit != poppedLit ){
	//in reasonclause for lit, don't treat this "poppedLit" 
	if( model_lit_is_of_current_dl( ad->model, lit ) ){
	  if ( !kv_A( ad->varMarks, var(lit) ) ){
	    kv_A( ad->varMarks, var(lit) ) = true;
	    numLitsThisDLPending++;
	  }
	} else if ( !kv_A(ad->varMarks, var(lit) ) && model_get_lit_DL(ad->model,lit) > 0 ) {
	  kv_A(ad->varMarks, var(lit)) = true;  // Note: we ignore dl-zero lits
	  ad->lemma.size++;
	  kv_A(ad->emma.lits, ad->lemma.size) = lit; // lower-dl-lit marked->already in lemma
	}
      }
    }

    do { // pop and unassign until finding next marked literal in the stack
      poppedLit = model_pop_and_set_undef(ad->model); 
      dassert(poppedLit!=0);
      azuDICI_notify_unassigned_lit(ad, poppedLit);
    } while ( !kv_A(ad->varMarks, var(poppedLit)) );
    kv_A(ad->varMarks, var(poppedLit)) = false;

    r = model_get_reason_of_Lit(ad->model, poppedLit);
    azuDICI_increase_activity(r);
    cl = azuDICI_get_clause_from_reason(r);

    if ( numLitsThisDLPending == 1) {
      break;  // i.e., only 1 lit this dl pending: this last lit is the 1UIP.
    }
    numLitsThisDLPending--;
    index=0;
  }

  dassert(numLitsThisDLPending==1);
  kv_A(ad->lemma.lits,0) = -poppedLit; //THe 1UIP is the first lit in the lemma
  ad->lemma.size++; 

  for (i=0;i<numLitsInLemma;i++) {
    kv_A( ad->varMarks, var(kv_A(ad->lemma,i)) ) = false;
  }

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
  Clause cl = NULL;
  uint j, lastMarkedInLemma, testingIndex;
  Literal litOfLemma, testingLit;
  uint i, lowestDL;
  Var v;
  bool litIsRedundant;
  Reason r;

  ad->lemmaToLearn.size = 0;
  if (ad->model.decisionLevel == 0) return;
 
  //First of all, mark all lits in original lemma.
  for (i=0;i<ad->lemma.size;i++) 
    kv_A( varMarks, kv_A(ad->lemma.lits,i) )=true;

  //Order lemma's lit from most recent to oldest (from highest dl to lowest)
  azuDICI_sort_lits_according_to_DL_from_index(ad->model, ad->lemma,1);
  lowestDL = model_get_lit_DL(ad->model, kv_A(ad->lemma, ad->size-1) );

  ad->dlToBackjump = 0; 
  ad->dlToBackjumpPos=0; 

  stats.totalLearnedLits+=numLitsInLemma;

  ad->lemmaToLearn.size = 0;

  //Go through the lits in lemma (except the UIP - lemma[0])
  // and test if they're redundant
  for( i=1; i < ad->lemma.size; i++ ){
    litIsRedundant=true;

    litOfLemma = kv_A(ad->lemma.lits,i);
    dassert( varMarks.isMarked(var(litOfLemma)) );

    //Begins test to see if literal is redundant
    if( model_lit_is_decision(ad->model, litOfLemma) || i==0 ){
      //If it's decision or the UIP, lit is not Redundant 
      litIsRedundant=false;
    }else{
      //We add reasons' lits at the end of the lemma
      //      r = model.reasonOfLit(litOfLemma.negation());
      r = model_get_reason_of_Lit(ad->model, poppedLit);
      azuDICI_get_clause_from_reason(r, &cl);

      //add literals of lit's reason to test
      lastMarkedInLemma = ad->lemma.size;
      for( j=0 ; j<cl.size ; j++ ){
	v = var( kv_A(cl->lits, j));
	if(kv_A(ad->varMarks, v)) continue;

	kv_A(ad->varMarks, v) = true;
	kv_A(ad->lemma, lastMarkedInLemma++) = kv_A(cl->lits,j);
      }

      //test added literals and subsequent ones....
      testingIndex = ad->lemma.size;
      while(testingIndex < lastMarkedInLemma){
	testingLit = kv_A(ad->lemma.lits,testingIndex);
	dassert(kv_A(ad->varMarks, var(testingLit)));
	if ( model_get_lit_DL(ad->model, testingLit) < lowestDL || //has lower dl
	     model_lit_is_decision(testingLit) ) { //is decision
	  //test fails
	  litIsRedundant=false;
	  break;
	}
      
	r = model_get_reason_of_lit(-testingLit);
	azuDICI_get_clause_from_reason(r, &cl);

	//cl = clauseReasonOfLit(testingLit.negation());
	//If testing lit passes test, add its reason's literals to test
	for(j=0;j<cl.size;j++){
	  v = var( kv_A(cl.lits,j) );	
	  if(kv_A(ad->varMarks,v)) continue;
	
	  kv_A(ad->varMarks,v) = true;
	  kv_A(ad->lemma,lastMarkedInLemma++) = kv_A(cl.lits, j);
	}
	testingIndex++;
      }
      if(testingIndex==lastMarkedInLemma) dassert(litIsRedundant);
      //Test ends only if a) Some unit or decision is reached
      //or b) all the reason's lits are already marked

      //Clean tested literals
      while ( lastMarkedInLemma > ad->lemma.size ) {
	kv_A(ad->varMarks,--lastMarkedInLemma)=false;
	//varMarks.setUnMarked(var(lemma[--lastMarkedInLemma]));
      }
    }

    //Add (or not) litOfLemma to lemmaToLearn
    if(litIsRedundant){
      kv_A(ad->lemma,i)=0;
      kv_A(ad->varMarks,var(litOfLemma))=false;
    }else{
      kv_A(ad->lemmaToLearn.lits, ad->lemmaToLearn.size) = litOfLemma;
      //Keep track of literal with highest dl besides de UIP
      if ( ad->lemmaToLearn.size > 0 && ad->dlToBackjump < model_get_lit_DL(ad->model, litOfLemma) ){
	ad->dlToBackjump = model_get_lit_DL(ad->model, litOfLemma);
	ad->dlToBackjumpPos = ad->lemmaToLearn.size;  //dlpos is position in lemma of maxDLLit
      }
      ad->lemmaToLearn.size++;
    }
  }

  /*Put in lemmaToLearn[1] the lit with highest dl*/
  Literal aux = kv_A(ad->lemmaToLearn.lits, ad->dlToBackjumpPos);
  kv_A(ad->lemmaToLearn.lits, ad->dlToBackjumpPos) = 
    kv_A(ad->lemmaToLearn.lits, 1);

  kv_A(ad->lemmaToLearn.lits, 1) = aux;


  //  stats.totalRemovedLits += (numLitsInLemma - numLitsInLemmaToLearn);

  for (i=0;i<numLitsInLemmaToLearn;i++){
    kv_A(ad->varMarks, var(kv_A(ad->lemmaToLearn.lits,i)) )=false;
  }

  /*  if(verboseMin)
    if (!(stats.numConflicts%10000))
    cout<<"total lits: "<<stats.totalLearnedLits<<" removed: "<<stats.totalRemovedLits<<"("<<(100*stats.totalRemovedLits)/stats.totalLearnedLits<<"pct)"<<endl;*/

}

void azuDICI_learn_lemma(AzuDICI* ad){
  //learn clause stored in ad->lemmaToLearn
  //store in ad->rUIP the reason to set the uip
  //If nclause, the 2 watched literals are the uip and the highest dl one.

  switch(ad->lemmaToLearn.size){
  case 1:
    insert_unitary_clause(ad->cdb, ad->lemmaToLearn, false);
    ad->rUIP.size     = 1;
    ad->rUIP.binLit   = 0;
    ad->rUIP.tClPtr   = NULL;
    ad->rUIP.thNClPtr = NULL;
    dassert(ad->dlToBackjump==0);
    break;

  case 2:
    Literal l1,l2;
    l1 = kv_A(ad->lemmaToLearn.lits,0);
    l2 = kv_A(ad->lemmaToLearn.lits,1);
    kv_A(ad->lastBinariesAdded,lit_as_uint(-l1))++; //hack for same search
    kv_A(ad->lastBinariesAdded,lit_as_uint(-l2))++; //hack for same search

    insert_binary_clause(ad->cdb, ad->lemmaToLearn, false);
    ad->rUIP.size     = 2;
    ad->rUIP.binLit   = kv_A(ad->lemmaToLearn,1);
    ad->rUIP.tClPtr   = NULL;
    ad->rUIP.thNClPtr = NULL;
    break;

  case 3:
    Lit *ptr = insert_ternary_clause(ad->cdb, ad->lemmaToLearn, false);
    ad->rUIP.size     = 3;
    ad->rUIP.binLit   = 0;
    ad->rUIP.tClPtr   = ptr;
    ad->rUIP.thNClPtr = NULL;
    break;

  default:
    Literal uip    = kv_A(ad->lemmaToLearn.lits,0);
    Literal hdlLit = kv_A(ad->lemmaToLearn.lits,1);

    dassert(ad->lemmaToLearn.size>3);
    //here, lemmaToLearn will be sorted
    unsigned int indexInDB = 
      insert_nary_clause(ad->cdb, ad->lemmaToLearn, false);

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
    Literal aux;
    for(int i=1; i < ad->lemmaToLearn.size; i++){
      if( kv_A(ad->lemmaToLearn.lits,i) == hdlLit ){
	dassert(model_get_lit_DL(kv_A(ad->lemmaToLearn.lits,i)) == ad->dlToBackjump);
	kv_A(ad->lemmaToLearn.lits,i) = kv_A(ad->lemmaToLearn.lits,1);
	kv_A(ad->lemmaToLearn.lits,1) = hdlLit;
	break;
      }
    }

    ThNClause *ptr = 
      azuDICI_insert_th_clause(ad->thcdb, ad->lemmaToLearn, false, indexInDB);

    ad->rUIP.size     = ad->lemmaToLearn.size;
    ad->rUIP.binLit   = 0;
    ad->rUIP.tClPtr   = NULL;
    ad->rUIP.thNClPtr = ptr;
    //Nclause
  }
}

void azuDICI_backjump_to_dl(AzuDICI* ad, unsigned int dl){
  Literal lit;
  uint dL1 = ad->model.decisionLevel;

  //  if (dl==0) stats.numConflictsSinceLastDLZero=0;
  while ( dL1>dl ){
    lit = model_pop_and_set_undef();
    while ( lit != 0 ) {
      azuDICI_notify_unassigned_lit(lit);
      lit=model_pop_and_set_undef();
    }
    dL1--;
  }
  ad->model.decisionLevel = dl;
}

void azuDICI_set_true_uip(AzuDICI* ad){
  if(ad->lemmaToLearn.size==1) {
    ad->stats.numDlZeroLitsSinceLastRestart++;
    ad->stats.numDLZeroLits++;
  }

  model_set_true_due_to_reason(ad->model, kv_A(ad->lemmaToLearn.lits,0), ad->rUIP);
  //conflict ends, and then we propagate
  ad->conflict.size = 0;
}

void azuDICI_clause_cleanup_if_adequate(AzuDICI* ad){
  if(conditionCleanup){

  }
}

void azuDICI_restart_if_adequate(AzuDICI* ad){
  if(conditionRestart){
    ad->stats.numRestarts++;
    ad->stats.numDLZeroLitsSinceLastRestart = 0;
    ad->stats.numConflictsSinceLastRestart  = 0;
    azudDICI_backjump_to_dl(ad,  0 );
  }
}

Literal  azuDICI_decide(AzuDICI* ad){
  int i,v=0;

  if ( (ad->model.decisionLevel < ad->strat.DLBelowWhichRandomDecisions) &&
       !(ad->stats.numDecisions % strat.fractionRandomDecisions) ){
    unsigned int rNumber= ad->cdb->randomNumbers[ad->randomNumberIndex++];
    v = (uint) ( (double)rNumber/RAND_MAX * ad->cdb.numVars ) + 1;
    for( i=1;i<1000;i++ ) {
      if (model.isUndefVar(v)) break;
      v = (v+1) % ad->cdb.numVars +1;
    }
    if ( !model_is_undef_var(ad->model,v) ) v=0;
  }

  if (!v) do {
      v=maxHeap_remove_max(ad->heap);
      if (!v) return(0);
    } while (!model_is_undef_var(ad->model, v));

  stats.numDecisions++;
  
  dassert(model_is_undef_var(ad->model, v));
  
  if( ad->strat.phaseSelectionDLParity ){
    if( ad->model.decisionLevel % 2 ) return -v;
    else return v;
  } else if( ad->strat.phaseSelectionLastPhase ){
    if( ad->model_get_lastPhase(ad->model,v) ) return v;
    else return -v;
  } else if( ad->strat.phaseSelectionAlwaysPositive ){
    return v
  }  else if( strat.phaseSelectionAlwaysNegative ){
    return -v;
  }
  
  return -v;  // default is always negative
}

bool azuDICI_propagate_w_binaries(AzuDICI* ad, Literal l){
  Literal lToSetTrue;
  unsigned int sizeOfList, litIndex;
  litIndex = lit_as_uint(l);
  ts_vec_t(Literal) list = kv_A(ad->bDB,litIndex);
  sizeOfList = kv_A(ad->lastBinariesAdded,litIndex); //hack for same search
  //  ts_vec_size(sizeOfList, list);
  Reason r;
  r.size = 2;
  r.tClPtr = NULL;
  r.thNClPtr = NULL;

  for(int i=0;i<sizeOfList;i++){
    ts_vec_ith(lToSetTrue, list, i);
    if (model_is_undef(lToSetTrue)){
      r.binLit = lToSetTrue;
      model_set_true_w_reason(ad->model,lToSetTrue,r);
      if(model->decisionLevel ==0){
	ad->stats.numDLZeroLits++;
	ad->stats.numDLZeroLitsSinceLastRestart++;
      }
    }else if(model_is_false(lToSetTrue)){ //Conflict
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
  unsigned int indexInTDB;
  Literal lToSetTrue;
  unsigned int sizeOfList;
  unsigned int litIndex = lit_as_uint(-l);
  ts_vec_t(unsigned int) list = kv_A(ad->tDB,litIndex);
  ts_vec_size(sizeOfList, list);
  Reason r;
  r.size = 3;
  r.binLit = 0;
  r.thNClPtr = NULL;
  TClause ternaryClause;
  Literal l1,l2,l3;
  for(i=0;i<sizeOfList;i++){
    ts_vec_ith(indexInTDB, list, i);
    ts_vec_ith(ternaryClause,ad->tDB,indexInTDB);
    if(ternaryClause.flags[ad->wId]){ //Only propagate with self-learned clauses
      l1 = ternaryClause.lits[0];
      l2 = ternaryClause.lits[1];
      l3 = ternaryClause.lits[2];
      lToSetTrue = 0;
      if(model_is_false(l1) && model_is_false(l2)){
	lToSetTrue = l3;
      }else if (model_is_false(l2) && model_is_false(l3)){
	lToSetTrue = l1;
      }else if (model_is_false(l1) && model_is_false(l3)){
	lToSetTrue = l2;
      }else{
	continue;
      }

      dassert(lToSetTrue>0);
      if (model_is_undef(lToSetTrue)){
	r.tClPtr = ternaryClause.lits; //check if this works
	model_set_true_w_reason(ad->model,lToSetTrue,r);
	if(model->decisionLevel ==0){
	  ad->stats.numDLZeroLits++;
	  ad->stats.numDLZeroLitsSinceLastRestart++;
	}
      }else if(model_is_false(lToSetTrue)){ //Conflict
	ad->conflict.size = 3;
	kv_A(ad->conflict.lits,0) = l1;
	kv_A(ad->conflict.lits,1) = l2;
	kv_A(ad->conflict.lits,2) = l3;
	return false;
      }    
    }
  }
  return true;
}

bool azuDICI_propagate_w_n_clauses(AzuDICI* ad, Literal l){
  int i;
  ThNClause** ptrToWatchedClauseAddr =  &kv_A(ad->watches,lit_as_uint(-l));
  //  ThNClause** previouslyWatchedClause = &kv_A(ad->watches,lit_as_uint(-l));
  ThNClause* watchedClause = kv_A(ad->watches,lit_as_uint(-l));
  bool first;
  unsigned int sizeOfClause;
  Literal otherLit, toReselect;
  Reason r;
  r.binLit = 0;
  r.tClPtr = NULL;
  bool foundForReselection;

  while(watchedClause){
    if (watchedClause->lwatch1 == -l){
      first=true;
      otherLit = watchedClause->lwatch2;
    } else{
      dassert(watchedClause->lwatch2 == -l);
      first=false;
      otherLit = watchedClause->lwatch1;
    }
    sizeOfClause = kvec_size(watchedClause->lits);

    if(model_is_true(otherLit)){
      //Update watchedLiteral and previouslyWatchedLiteral
      if(first){
	ptrToWatchedClauseAddr = &watchedClause->nextWatched1;
	//watchedClause = watchedClause->nextWatched1; see below, is the same
      }else{
	ptrToWatchedClauseAddr = &watchedClause->nextWatched2;
	//watchedClause = watchedClause->nextWatched2; see below, is the same
      }
      watchedClause = *ptrToWatchedClauseAddr;
      continue;
    }
    
    //if other watched is not true, try to reselect.
    foundForReselection=false;
    for(i=0;i<sizeOfClause;i++){
      toReselect = kv_A(watchedClause->lits, i);
      if( toReselect!=-l && toReselect != otherLit && 
	  model_is_true_or_undef(toReselect)){ //found to reselect
	foundForReselection =true;
	break;
      }
    }

    //    if( i < sizeOfClause){  //Found lit to reselect
    if( foundForReselection ){  //Found lit to reselect
      dassert(model_is_false(-l));
      if ( first ) {
	watchedClause->lwatch1 = toReselect;
	//what stores this direction, is overwritten 
	*ptrToWatchedClauseAddr = watchedClause->nextWatched1; 
	watchedClause->nextWatched1 = kv_A(ad->watches,lit_as_uint(toReselect));
      } else {
	watchedClause->lwatch2 = toReselect;
	//what stores this direction, is overwritten 
	*ptrToWatchedClauseAddr = watchedClause->nextWatched2; 
	watchedClause->nextWatched2 = kv_A(ad->watches,lit_as_uint(toReselect))
      }
      //update previouslyWatchedClause
      kv_A(ad->watches, lit_as_uint(toReselect)) = watchedClause;
      watchedClause = *ptrToWatchedClauseAddr;
      continue;
    }else{//Propagate otherLit or conflict
      dassert(i==sizeOfClause);
      if (model_is_undef(otherLit)){ //Propagate
	r.size = sizeOfClause;
	r.thNClPtr = watchedClause;
	model_set_true_w_reason(ad->model,otherLit,r);
	if(model->decisionLevel ==0){
	  ad->stats.numDLZeroLits++;
	  ad->stats.numDLZeroLitsSinceLastRestart++;
	}
	//update watchedClause and pointer to it.
	if(first){
	  ptrToWatchedClauseAddr = &watchedClause->nextWatched1;
	}else{
	  ptrToWatchedClauseAddr = &watchedClause->nextWatched2;
	}
	watchedClause = *ptrToWatchedClauseAddr;
	continue;
      }else if(model_is_false(otherLit)){ //Conflict
	//Remember to update activity
	watchedClause->activity++;
	ad->conflict.size = sizeOfClause;
	for(i=0;i<sizeOfClause;i++){
	  kv_A(ad->conflict.lits,i) = kv_A(watchedClause->lits,i);
	}
	return false;
      }    
    }
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
  threadClause.nextWatched1  = (void*)kv_A(ad->watches, lit_as_uint(-l1)); 
  threadClause.nextWatched2  = (void*)kv_A(ad->watches, lit_as_uint(-l2));

  NClause clInDB;
  ts_vec_ith(clInDB, ad->cdb.nDB, indexInDB);
  threadClause.lits      = &clInDB.lits;
  //  threadClause.posInDB      = posInDB; Do we need this, rethink?

  kvec_push(ThNClause, ad->thcdb, threadClause);

  //insert at beginning of watches linked lists.
  ThNClause* ptrToThClause = &kv_A(ad->thcdb, kvec_size(ad->thcdb)-1);
  kv_A(ad->watches, lit_as_uint(-l1)) = ptrToThClause;
  kv_A(ad->watches, lit_as_uint(-l2)) = ptrToThClause;

  return ptrToThClause;
}
#endif
