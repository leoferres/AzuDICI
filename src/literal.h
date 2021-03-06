#ifndef _LITERAL_H_
#define _LITERAL_H_

#include <stdbool.h>
#include "common.h"

typedef int Literal; /* A literal is just an int. */
unsigned int lit_as_uint(Literal l);
Literal zero_lit(); /*returns the zero lireal*/
bool lit_is_positive(Literal lit); /*returns true if the literal is positive, false otherwise*/
unsigned int uint_as_lit(Literal l);



#endif /* _LITERAL_H_ */
