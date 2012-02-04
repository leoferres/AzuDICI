#include <stdbool.h>
#include <assert.h>
#include "literal.h"

/*debug assert*/
#define DEBUG 1
#if DEBUG
#define dassert(n) assert(n)
#else
#define dassert(n);
#endif

typedef unsigned int Var;

Var var(Literal l);