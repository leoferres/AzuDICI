#ifndef _STRATEGY_C_
#define _STRATEGY_C_

#include "strategy.h"
#include "math.h"

#define is_zero_or_power_of_two_2(x)  ( !(((x)-1) & (x)) )

void strategy_init(Strategy strat, int workerId){
  switch(workerID){ //here, we will configure different strategies for each worker
  default:
    initialScoreBonus             = 1.0;
    scoreBonusInflationFactor     = 1.125;
    DLBelowWhichRandomDecisions   = 10;
    fractionRandomDecisions;      = 200; // 1/k times;
    decideOverLits;               = false;
    phaseSelectionDLParity;       = false;
    phaseSelectionLastPhase;      = true;
    phaseSelectionAlwaysPositive  = false;
    phaseSelectionAlwaysNegative  = false; 
    lubyNumbersMultiplier         = 75;
    initialRestartLimit           = lubyNumberMultiplier;
  }
}

unsigned int strategy_get_next_restart_limit(Strategy strat, unsigned int currentRestartLimit){
  unsigned int lNumber = currentRestartLimit / strat->lubyNumbersMultiplier;
  if(is_zero_or_power_of_two_2(lNumber+1)){
    return((unsigned int)strat->lubyNumber * pow(2,log2(lNumber+1)-1));
  } else{
    uint l = (int)log2(lNumber);
    uint p = (int)pow((float)2,(int)l);
    return(strategy_get_next_restart_limit(strat, lNumber+1-p));
  }
}

#endif
