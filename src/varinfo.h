#include "reason.h"
#include "stdbool.h"

typedef struct _vinfo {
  Reason r;
  unsigned int decision_lvl;
  unsigned int model_height;
  bool last_phase;
  bool propagated;
} VarInfo;
