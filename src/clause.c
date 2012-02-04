#include "clause.h"

Reason no_reason(){
    Reason r;
    r.reason=1;
    return r;
}

bool is_unit_clause(Reason r){
    return r.reason==1;
}
