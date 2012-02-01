#ifndef _AZUDICI_C_
#define _AZUDICI_C_

#include "clausedb.h"
#include "azudici.h"

AzuDICI* azuDICI_init(ClauseDB* generalClauseDB){
  int i;

  AzuDICI *ad = (AzuDICI*)malloc(sizeof(AzuDICI));
  unsigned int  dbSize;
  ad->cdb                = generalClauseDB;
  ts_vec_size( dbSize, db->uDB );
  ad->lastUnitAdded      = dbSize;
  ts_vec_size( dbSize, db->nDB );
  ad->lastNaryAdded      = dbSize;
  ad->randomNumberIndex  = 0;
  ad->dlToBackjump       = 0;
  ad->decisionLevel      = 0;

  kvec_init( lastBinariesAdded );
  kvec_resize( 2*(ad->cdb->numVars+1) );
  int listSize;
  for( i=0; i < 2*(ad->cdb->numVars + 1) ; i++ ){
    ts_vec_size( listSize, kvec_A(ad->cdb->bDB, i) );
    kvec_A(ad->lastBinariesAdded, i) = listSize-1;
  }

  state_init(ad->state);
  watches_init(ad->watches, ad->db);
  model_init(ad->model);
  heap_init(ad->model);

  kvec_init(conflict.lits);
  kvec_resize(Literal, conflict.lits, ad->db->numVars);
  conflict.size = 0;

  kvec_init(lemma.lits);
  kvec_resize(Literal, lemma.lits, ad->db->numVars);
  lemma.size = 0;

  kvec_init(lemmaToLearn.lits);
  kvec_resize(Literal, lemmaToLearn.lits, ad->db->numVars);
  lemmaToLearn.size = 0;

  stats_init(ad->stats);
  return ad;
}

unsigned int azuDICI_solve(AzuDICI* ad){
  //setTrue at dl 0 all unit clauses
  if( !azuDICI_set_true_units(ad) ) return 20;
  Literal dec;
  Reason r;

  while (true) {
    while( !azuDICI_propagate(ad) ){ //While conflict in propagate
      if( ad->decisionLevel == 0 ) return 20;
      azuDICI_conflict_analysis(ad);
      azuDICI_lemma_shortening(ad);
      azuDICI_learnLemma(ad);
      azuDICI_backjump(ad);
      azuDICI_set_true_uip(ad);
    }
    
    azuDICI_restart_if_adequate(ad);
    if( !azuDICI_cleanup_if_adequate(ad) ) return 20;
    
    dec = azuDICI_get_next_decision(ad);
    if (dec == 0)  return 10;
    
    azuDICI_set_true_due_to_decision( ad, dec );
  }
  
  return 0;
}

bool azuDICI_propagate( ad ){
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
  scoreBonus *= strat.scoreBonusInflationFactor;
  ad->lemma.size = 0;
  
  uint i, numLitsThisDLPending=0;
  Literal lit, poppedLit=0;
  uint index=0;
  Clause cl = ad->conflict;
  Reason r;
  
  while(true){
    //cl->increaseActivity(); This should be incremented when processing the reason or when conflict detected......
    
    while( index < cl.size ){//traverse conflictingclause/currentreasonclause,
      lit = kvec_A( cl.lits, index ); 
      index++; // marking literals and counting lits of this DL
      azuDICI_increaseScore(ad, lit);
      if( lit != poppedLit ){
	//in reasonclause for lit, don't treat this "poppedLit" 
	if( model_lit_is_of_current_dl( ad->model, lit ) ){
	  if ( !kvec_A( ad->varMarks, var(lit) ) ){
	    kvec_A( ad->varMarks, var(lit) ) = true;
	    numLitsThisDLPending++;
	  }
	} else if ( !kvec_A(ad->varMarks, var(lit) ) && model_get_lit_DL(ad->model,lit) > 0 ) {
	  kvec_A(ad->varMarks, var(lit)) = true;  // Note: we ignore dl-zero lits
	  ad->lemma.size++;
	  kvec_A(ad->emma.lits, ad->lemma.size) = lit; // lower-dl-lit marked->already in lemma
	}
      }
    }

    do { // pop and unassign until finding next marked literal in the stack
      poppedLit = model_pop_and_set_undef(ad->model); 
      dassert(poppedLit!=0);
      azuDICI_notify_unassigned_lit(ad, poppedLit);
    } while ( !kvec_A(ad->varMarks, var(poppedLit)) );
    kvec_A(ad->varMarks, var(poppedLit)) = false;

    r = model_get_reason_of_Lit(ad->model, poppedLit);
    azuDICI_increase_activity(r);
    azuDICI_get_clause_from_reason(r, &cl);

    if ( numLitsThisDLPending == 1) {
      break;  // i.e., only 1 lit this dl pending: this last lit is the 1UIP.
    }
    numLitsThisDLPending--;
    index=0;
  }

  dassert(numLitsThisDLPending==1);
  kvec_A(ad->lemma.lits,0) = -poppedLit; //THe 1UIP is the first lit in the lemma
  ad->lemma.size++; 

  for (i=0;i<numLitsInLemma;i++) {
    kvec_A( ad->varMarks, var(kvec_A(ad->lemma,i)) ) = false;
  }

}

/***************TO IMPLEMENT****************/
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
  if (stats.decisionLevel==0) return;
 
  //First of all, mark all lits in original lemma.
  for (i=0;i<ad->lemma.size;i++) 
    kvec_A( varMarks, kvec_A(ad->lemma.lits,i) )=true;

  //Order lemma's lit from most recent to oldest
  azuDICI_sort_lits_according_to_DL_from_index(ad->model, ad->lemma,1);
  lowestDL = model_get_lit_DL(ad->model, kvec_A(ad->lemma, ad->size-1) );

  /*  mod = &model;
  sort( ((Lit *)lemma)+1, ((Lit *)lemma)+numLitsInLemma, cmp2 );
  lowestDL = model.litDL(lemma[numLitsInLemma-1]);*/

  //for(i=1;i<numLitsInLemma;i++){
  //if(model.litDL(lemma[i])<lowestDL){
  //  lowestDL = model.litDL(lemma[i]);
  // }
  //}

  ad->dlToBackjump = 0; 
  ad->dlToBackjumpPos=0; 

  stats.totalLearnedLits+=numLitsInLemma;

  ad->lemmaToLearn.size = 0;

  //Go through the lits in lemma (except the UIP - lemma[0])
  // and test if they're redundant
  for( i=1; i < ad->lemma.size; i++ ){
    litIsRedundant=true;

    litOfLemma = kvec_A(ad->lemma.lits,i);
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
      /*if (r.isBinClause()) {
	binClauseShortening.setIthLit(0,litOfLemma.negation());
	binClauseShortening.setIthLit(1,r.extractBinLit());
	binClauseShortening.setNumLits(2);
	cl = &binClauseShortening;
      } else if (r.isNClause())  cl = r.extractNClause();
      //cl = clauseReasonOfLit(litOfLemma.negation());*/

      //add literals of lit's reason to test
      lastMarkedInLemma = ad->lemma.size;
      for( j=0 ; j<cl.size ; j++ ){
	v = var( kvec_A(cl->lits, j));
	if(kvec_A(ad->varMarks, v)) continue;

	kvec_A(ad->varMarks, v) = true;
	kvec_A(ad->lemma, lastMarkedInLemma++) = kvec_A(cl->lits,j);
      }

      //test added literals and subsequent ones....
      testingIndex = ad->lemma.size;
      while(testingIndex < lastMarkedInLemma){
	testingLit = kvec_A(ad->lemma.lits,testingIndex);
	dassert(kvec_A(ad->varMarks, var(testingLit)));
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
	  v = var( kvec_A(cl.lits,j) );	
	  if(kvec_A(ad->varMarks,v)) continue;
	
	  kvec_A(ad->varMarks,v) = true;
	  kvec_A(ad->lemma,lastMarkedInLemma++) = kvec_A(cl.lits, j);
	}
	testingIndex++;
      }
      if(testingIndex==lastMarkedInLemma) dassert(litIsRedundant);
      //Test ends only if a) Some unit or decision is reached
      //or b) all the reason's lits are already marked

      //Clean tested literals
      while ( lastMarkedInLemma > ad->lemma.size ) {
	kvec_A(ad->varMarks,--lastMarkedInLemma)=false;
	//varMarks.setUnMarked(var(lemma[--lastMarkedInLemma]));
      }
    }

    //Add (or not) litOfLemma to lemmaToLearn
    if(litIsRedundant){
      kvec_A(ad->lemma,i)=0;
      kvec_A(ad->varMarks,var(litOfLemma))=false;
      //      varMarks.setUnMarked(var(litOfLemma));
    }else{
      kvec_A(ad->lemmaToLearn.lits, ad->lemmaToLearn.size) = litOfLemma;
      //Keep track of literal with highest dl besides de UIP
      if ( ad->lemmaToLearn.size > 0 && ad->dlToBackjump < model_get_lit_DL(ad->model, litOfLemma) ){
	ad->dlToBackjump = model_get_lit_DL(ad->model, litOfLemma);
	ad->dlToBackjumpPos = ad->lemmaToLearn.size;  //dlpos is position in lemma of maxDLLit
      }
      ad->lemmaToLearn.size++;
    }
  }

  //  stats.totalRemovedLits += (numLitsInLemma - numLitsInLemmaToLearn);

  for (i=0;i<numLitsInLemmaToLearn;i++){
    kvec_A(ad->varMarks, var(kvec_A(ad->lemmaToLearn.lits,i)) )=false;
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
    break;
  case 2:
    break;
  case 3:
    break;
  default:
    //Nclause
  }
}

#endif
