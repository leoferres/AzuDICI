#include "literal.h"

unsigned int uint_as_lit(Literal l) {
  //dassert(l!=0);
  if(l%2) return -l/2;
  else return (l/2);  
}

unsigned int lit_as_uint(Literal l) {
  //  dassert(l!=0); 
  if(l < 0) return -l*2;
  else return l*2+1;
}

/* Return the 0 literal*/
inline Literal zero_lit(){
    return 0;
}

/*Check if a literal is positive*/
inline bool lit_is_positive(Literal lit){
  return (lit>=0);
}
