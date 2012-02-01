#include <stdbool.h>
#include <assert.h>

/*debug assert*/
#define DEBUG 1
#if DEBUG
#define dassert(n) assert(n)
#else
#define dassert(n);
#endif

typedef unsigned int Var;

inline Var var(Literal l) {
    if(l<0){
        return (unsigned int)(l*-1);
    }
    return (unsigned int)l;
}
 
