#ifndef _STATS_H_
#define _STATS_H_

#include "stats.h"

void stats_init(Stats st){
  numConflictsSinceClauseCleanup = 0;
  numConflictsSinceLastRestart   = 0;
  numConflictsSinceLastDLZero    = 0;
  numConflicts                   = 0;
  numDLZeroLits                  = 0;
  numDlZeroLitsSinceLastRestart  = 0;
  numRestarts                    = 0;
  numDecisions                   = 0;
}

#endif
