typedef struct _strategy{
  double initialScoreBonus;
  double scoreBonusInflationFactor;
  unsigned int DLBelowWhichRandomDecisions;
  double fractionRandomDecisions;
  bool decideOverLits;
  bool phaseSelectionDLParity;
  bool phaseSelectionLastPhase;
  bool phaseSelectionAlwaysPositive;
  bool phaseSelectionAlwaysNegative;
}Strategy;

void strategy_init(Strategy strat, int workerId);
