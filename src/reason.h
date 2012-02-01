#include "clause.h"

enum RTYPE {
  NCLAUSE,
  TCLAUSE,
  LITERAL
};

typedef struct _reason {
  NClause* nclause;
  TClause* tclause;
  Literal bliteral; /*implied binary literal*/
  enum RTYPE w; /*syntax of enums???*/
} Reason;
