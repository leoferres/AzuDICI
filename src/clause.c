#include "clause.h"

Reason no_reason(){
    Reason r;
    r.size=1;
    return r;
}

bool is_unit_clause(Reason r){
    return r.size==1;
}
