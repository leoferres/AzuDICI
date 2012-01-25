#include "literal.h"

unsigned int lit_as_uint(Literal l) {

  dassert(l!=0);
  
  if(l < 0)
    return -l*2;
  else
    return l*2+1;

}
