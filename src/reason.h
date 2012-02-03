#include "clause.h"

enum RTYPE {
  NCLAUSE,
  TCLAUSE,
  LITERAL
};

typedef struct _reason {
  NClause* nClPtr;
  TClause* tClPtr;
  ThNClause* thNClPtr;
  Literal binLit; /*implied binary literal*/
  unsigned int size;
  enum RTYPE w; /*syntax of enums???*/
} Reason;
