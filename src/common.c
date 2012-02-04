#include "common.h"

Var var(Literal l) {
    if(l<0){
        return (unsigned int)(l*-1);
    }
    return (unsigned int)l;
}