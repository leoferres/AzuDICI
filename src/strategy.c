#ifndef _STRATEGY_C_
#define _STRATEGY_C_

#include "strategy.h"

#define is_zero_or_power_of_two_2(x)  ( !(((x)-1) & (x)) )

void strategy_init( Strategy *strat, int workerId){
  switch(workerId){ //here, we will configure different strategies for each worker
case 1:
    strat->initialScoreBonus             = 1.0;
    strat->scoreBonusInflationFactor     = 1.125;
    strat->DLBelowWhichRandomDecisions   = 20;
    strat->fractionRandomDecisions       = 100; // 1/k times;
    strat->decideOverLits                = false;
    strat->phaseSelectionDLParity        = false;
    strat->phaseSelectionLastPhase       = false;
    strat->phaseSelectionAlwaysPositive  = true;
    strat->phaseSelectionAlwaysNegative  = false; 

    strat->initialCleanupLimit           = 10000;
    strat->cleanupMultiplier             = 1.01; 
    strat->activityThreshold             = 2;

    strat->lubyNumbersMultiplier         = 75;
    strat->initialRestartLimit           = strat->lubyNumbersMultiplier;
    break;

  case 2:
    strat->initialScoreBonus             = 1.0;
    strat->scoreBonusInflationFactor     = 1.125;
    strat->DLBelowWhichRandomDecisions   = 10;
    strat->fractionRandomDecisions       = 200; // 1/k times;
    strat->decideOverLits                = false;
    strat->phaseSelectionDLParity        = true;
    strat->phaseSelectionLastPhase       = false;
    strat->phaseSelectionAlwaysPositive  = false;
    strat->phaseSelectionAlwaysNegative  = false; 

    strat->initialCleanupLimit           = 15000;
    strat->cleanupMultiplier             = 1.05; 
    strat->activityThreshold             = 1;

    strat->lubyNumbersMultiplier         = 75;
    strat->initialRestartLimit           = strat->lubyNumbersMultiplier;
    break;

  case 3:
    strat->initialScoreBonus             = 1.0;
    strat->scoreBonusInflationFactor     = 1.125;
    strat->DLBelowWhichRandomDecisions   = 10;
    strat->fractionRandomDecisions       = 200; // 1/k times;
    strat->decideOverLits                = false;
    strat->phaseSelectionDLParity        = false;
    strat->phaseSelectionLastPhase       = true;
    strat->phaseSelectionAlwaysPositive  = false;
    strat->phaseSelectionAlwaysNegative  = false; 

    strat->initialCleanupLimit           = 15000;
    strat->cleanupMultiplier             = 1.05; 
    strat->activityThreshold             = 5;

    strat->lubyNumbersMultiplier         = 100;
    strat->initialRestartLimit           = strat->lubyNumbersMultiplier;
    break;

  case 4:
    strat->initialScoreBonus             = 1.0;
    strat->scoreBonusInflationFactor     = 1.125;
    strat->DLBelowWhichRandomDecisions   = 10;
    strat->fractionRandomDecisions       = 200; // 1/k times;
    strat->decideOverLits                = false;
    strat->phaseSelectionDLParity        = false;
    strat->phaseSelectionLastPhase       = true;
    strat->phaseSelectionAlwaysPositive  = false;
    strat->phaseSelectionAlwaysNegative  = false; 

    strat->initialCleanupLimit           = 15000;
    strat->cleanupMultiplier             = 1.05; 
    strat->activityThreshold             = 1;

    strat->lubyNumbersMultiplier         = 75;
    strat->initialRestartLimit           = strat->lubyNumbersMultiplier;
    break;
  case 5:
    strat->initialScoreBonus             = 1.0;
    strat->scoreBonusInflationFactor     = 1.125;
    strat->DLBelowWhichRandomDecisions   = 10;
    strat->fractionRandomDecisions       = 200; // 1/k times;
    strat->decideOverLits                = false;
    strat->phaseSelectionDLParity        = false;
    strat->phaseSelectionLastPhase       = true;
    strat->phaseSelectionAlwaysPositive  = false;
    strat->phaseSelectionAlwaysNegative  = false; 

    strat->initialCleanupLimit           = 15000;
    strat->cleanupMultiplier             = 1.05; 
    strat->activityThreshold             = 1;

    strat->lubyNumbersMultiplier         = 75;
    strat->initialRestartLimit           = strat->lubyNumbersMultiplier;

  default:
    strat->initialScoreBonus             = 1.0;
    strat->scoreBonusInflationFactor     = 1.125;
    strat->DLBelowWhichRandomDecisions   = 10;
    strat->fractionRandomDecisions       = 200; // 1/k times;
    strat->decideOverLits                = false;
    strat->phaseSelectionDLParity        = false;
    strat->phaseSelectionLastPhase       = true;
    strat->phaseSelectionAlwaysPositive  = false;
    strat->phaseSelectionAlwaysNegative  = false; 

    strat->initialCleanupLimit           = 15000;
    strat->cleanupMultiplier             = 1.05; 
    strat->activityThreshold             = 1;

    strat->lubyNumbersMultiplier         = 75;
    strat->initialRestartLimit           = strat->lubyNumbersMultiplier;
  }
}

unsigned int strategy_get_next_restart_limit(Strategy *strat, unsigned int currentRestartLimit){
  unsigned int lNumber = currentRestartLimit / strat->lubyNumbersMultiplier;
  if(is_zero_or_power_of_two_2(lNumber+1)){
    return((unsigned int)strat->lubyNumbersMultiplier * pow(2,log2(lNumber+1)-1));
  } else{
    unsigned int l = (int)log2(lNumber);
    unsigned int p = (int)pow((float)2,(int)l);
    return(strategy_get_next_restart_limit(strat, lNumber+1-p));
  }
}

#endif
