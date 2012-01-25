#ifndef _BINLIST_H_
#define _BINLIST_H_

#include "literal.h"

typedef struct _blist {
  Literal* lits;
  unsigned int last_elem;
  unsigned int size;
} BinaryList;

#endif /* _BINLIST_H_ */

