#ifndef _COMMON_H_
#define _COMMON_H_

#include <assert.h>

/*debug assert*/
#define DEBUG 0
#if DEBUG
#define dassert(n) assert(n)
#else
#define dassert(n);
#endif

/* typedef struct _pair_of_ints{ */
/*   unsigned int first; */
/*   unsigned int second; */
/* }UintPair; */

#endif
