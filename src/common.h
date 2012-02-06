#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdbool.h>
#include <assert.h>

#include "var.h"
#include "literal.h"

/*debug assert*/
#define DEBUG 1
#if DEBUG
#define dassert(n) assert(n)
#else
#define dassert(n);
#endif

#endif