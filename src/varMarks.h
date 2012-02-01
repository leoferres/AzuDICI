#ifndef VARMARKS_H
#define	VARMARKS_H
#include "kvec.h"
#include "common.h"

typedef struct _varMarks{
    kvec_t(bool) marked;
    unsigned int n_vars;
} VarMarks;

inline VarMarks init_VarMarks(unsigned int num_vars){
    VarMarks varMarks;
    kv_init(varMarks.marked);
    kv_resize(bool,varMarks.marked,num_vars+1);
    varMarks.n_vars = num_vars;
    for(Var v=1;v<=varMarks.n_vars;v++){
        kv_A(varMarks.marked,v)=false;
    }
}

inline void add_new_var(VarMarks varMarks){
    varMarks.n_vars++;
    kv_push(bool,varMarks.marked,false);
    dassert(kv_size(varMarks.marked)==int(varMarks.n_vars)+1);
}

inline void ensure_all_unmarked(VarMarks varMarks){
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

inline bool is_marked(Var v, VarMarks varMarks){
    if(DEBUG){
        if(!(1<=v && v<=varMarks.n_vars)){
            printf("Asking for var %i\n",v);
            printf("And n_vars is %i\n",varMarks.n_vars);
        }
    }
    dassert(1<=v && v<=varMarks.n_vars);
    return kv_A(varMarks.marked,v);
}

inline void set_marked(Var v, VarMarks varMarks){
    dassert(1<=v && v<=varMarks.n_vars);
    kv_A(varMarks.marked,v)=true;
}

inline void set_unmarked(Var v, VarMarks varMarks){
    dassert(1<=v && v<=varMarks.n_vars);
    kv_A(varMarks.marked,v)=false;
}

#endif	/* VARMARKS_H */
