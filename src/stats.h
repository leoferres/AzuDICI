typedef struct _statistics{
  unsigned int numConflictsSinceClauseCleanup;
  unsigned int numConflictsSinceLastRestart;
  unsigned int numConflictsSinceLastDLZero;
  unsigned int numConflicts;
  unsigned int numDLZeroLits;

  unsigned int numDlZeroLitsSinceLastRestart;
  unsigned int numRestarts;

  unsigned int numDecisions;
}Stats;

void stats_init(Stats st);

