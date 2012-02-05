#ifndef _STRATEGY_H_
#define _STRATEGY_H_
#include <stdbool.h>

typedef struct _strategy{
  double initialScoreBonus;
  double scoreBonusInflationFactor;
  unsigned int DLBelowWhichRandomDecisions;
  unsigned int fractionRandomDecisions;
  bool decideOverLits;
  bool phaseSelectionDLParity;
  bool phaseSelectionLastPhase;
  bool phaseSelectionAlwaysPositive;
  bool phaseSelectionAlwaysNegative;

  unsigned int initialRestartLimit;
  unsigned int lubyNumbersMultiplier;
}Strategy;

void strategy_init(Strategy strat, int workerId);
unsigned int strategy_get_next_restart_limit(Strategy strat, unsigned int currentRestartNumber);

#endif
