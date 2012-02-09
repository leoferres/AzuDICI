#ifndef _STATS_H_
#define _STATS_H_

typedef struct _statistics{
  unsigned int numConflictsSinceLastCleanup;
  unsigned int numConflictsSinceLastRestart;
  unsigned int numConflictsSinceLastDLZero;
  unsigned int numConflicts;
  unsigned int numDLZeroLits;

  unsigned int numDlZeroLitsSinceLastRestart;
  unsigned int numRestarts;
  unsigned int numCleanups;

  unsigned int numDecisions;
}Stats;

void stats_init(Stats *st);

#endif
