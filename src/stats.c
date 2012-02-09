#include "stats.h"

void stats_init(Stats *st){
  st->numConflictsSinceLastCleanup   = 0;
  st->numConflictsSinceLastRestart   = 0;
  st->numConflictsSinceLastDLZero    = 0;
  st->numCleanups                    = 0;
  st->numConflicts                   = 0;
  st->numDLZeroLits                  = 0;
  st->numDlZeroLitsSinceLastRestart  = 0;
  st->numRestarts                    = 0;
  st->numDecisions                   = 0;
}
