#ifndef _COMMON_H_
#define _COMMON_H_

#include <assert.h>

/*debug assert*/
#define DEBUG 1
#if DEBUG
#define dassert(n) assert(n)
#else
#define dassert(n);
#endif

#endif