#ifndef VARMARKS_H
#define	VARMARKS_H
#include "kvec.h"
#include "common.h"
#include <stdio.h>
#include "azudici.h"
#include "var.h"

typedef struct _varMarks{
    kvec_t(bool) marked;
    unsigned int n_vars;
} VarMarks;

VarMarks init_VarMarks(unsigned int num_vars){
    VarMarks varMarks;
    kv_init(varMarks.marked);
    kv_resize(bool,varMarks.marked,num_vars+1);
    varMarks.n_vars = num_vars;
    for(Var v=1;v<=varMarks.n_vars;v++){
        kv_A(varMarks.marked,v)=false;
    }
}

void add_new_var(VarMarks varMarks){
    varMarks.n_vars++;
    kv_push(bool,varMarks.marked,false);
    dassert(kv_size(varMarks.marked)==(varMarks.n_vars)+1);
}

void ensure_all_unmarked(VarMarks varMarks){
    bool there_are_marks = false;
    for(Var v=1; v<= varMarks.n_vars;++v){
        if(kv_A(varMarks.marked,v)){
            if(DEBUG){
                printf("ensure_all_unmarked() failure");
            } 
            there_are_marks = true;
        }       
    }
    dassert(!there_are_marks);
}

bool is_marked(Var v, AzuDICI* ad){
    if(DEBUG){
        if(!(1<=v && v<=ad->cdb->numVars)){
            printf("Asking for var %i\n",v);
            printf("And n_vars is %i\n",ad->cdb->numVars);
        }
    }
    dassert(1<=v && v<=ad->cdb->numVars);
    return kv_A(ad->varMarks,v);
}

void set_marked(Var v, VarMarks varMarks){
    dassert(1<=v && v<=varMarks.n_vars);
    kv_A(varMarks.marked,v)=true;
}

void set_unmarked(Var v, VarMarks varMarks){
    dassert(1<=v && v<=varMarks.n_vars);
    kv_A(varMarks.marked,v)=false;
}

#endif	/* VARMARKS_H */
