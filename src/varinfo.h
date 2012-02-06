#ifndef _VARINFO_H_
#define _VARINFO_H_

#include "stdbool.h"
#include "clause.h"

typedef struct _vinfo {
  Reason r;
  unsigned int decision_lvl;
  unsigned int model_height;
  bool last_phase;
  bool propagated;
} VarInfo;

#endif
