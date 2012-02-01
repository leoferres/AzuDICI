#include "model.h"

inline void push(Literal lit, Model model){
    kv_push(Literal,model.model_stack,lit);
}

inline Model init_model(unsigned int num_vars){
    Model model;
    model.n_vars = num_vars;
    model.n_lits = num_vars*2+2;
    kv_init(model.model_stack);
    kv_resize(Literal,model.model_stack,model.n_lits);
    model.last2propagated = -1;
    model.last3propagated = -1;
    model.lastNpropagated = -1;
    kv_init(model.vinfo);
    kv_resize(VarInfo,model.vinfo,model.n_vars+1);
    kv_resize(char,model.assignment),model.n_lits;
    model.vassignment = ((short unsigned *)((char *)model.assignment));
    model.dlMarker = zero_lit();
    model.decision_lvl = 0;
    for(unsigned int v=0;v<=model.n_vars;v++){
        init_in_assignment(v,model);
    }
    kv_size(model.model_stack)=0;
    return model;
}

inline void print(Model model){
  int i;
  Literal lit;
  for(i=kv_size(model.model_stack)-1;i>0;i--) {
    lit=kv_A(model.model_stack,i);
    if (lit != model.dlMarker) 
      printf("%i ",lit);
    else
      printf("===========");
    if (i==model.last2propagated) printf("  <-- last2 propagated");
    if (i==model.lastNpropagated) printf("  <-- lastN propagated");
    if (i==model.last3propagated) printf("  <-- lastT propagated");
    printf("\n");
  }
    printf("=============== bottom of stack ==============\n\n");     
}

inline Literal get_next_marked_literal(VarMarks var_marks, Model model){
  Literal lit;
  int i = kv_size(model.model_stack);
  do {
    lit = kv_A(model.model_stack,i);
   dassert(lit != model.dlMarker);
   i--;
  } while(!is_marked(var(lit)));
  return(lit);
}

/*  
 * true  is ......01   
 * false is ......00    
 * undef is ......11
*/

inline bool is_true(Literal lit, Model model){
    return (kv_A(model.assignment,lit_as_uint(lit)) & 0x03)==1;
}

inline bool is_false(Literal lit, Model model){
    dassert(var(lit)<=model.n_vars);
    return !(kv_A(model.assignment,lit_as_uint(lit)) & 0x03);
}

inline bool is_undef(Literal lit, Model model){
    return (kv_A(model.assignment,lit_as_uint(lit)) & 0x02);
}

inline bool is_true_or_undef(Literal lit, Model model){
    return (kv_A(model.assignment,lit_as_uint(lit)) & 0x03);
}

inline bool is_undef_var(Var v, Model model){
    return (kv_A(model.assignment,v) & 0x0202);
}

inline void set_true_in_assignment(Literal lit, Model model){
    model.vassignment[var(lit)] &= lit_is_positive(lit)?0xFDFC:0xFCFD; //D=1101 c=1100
}

inline void set_undef_in_assignment(Literal lit, Model model){
    model.vassignment[var(lit)] |= 0x0303;
}

inline void init_in_assignment(Var var, Model model){
    model.vassignment[var] = 0x0303;
}

inline void set_true_due_to_reason(Literal lit, Reason r, Model model){
    dassert(is_undef(lit,model));
    push(lit,model);
    set_true_in_assignment(lit,model);
    VarInfo *vi = &kv_A(model.vinfo,var(lit));
    vi->r=r;
    vi->decision_lvl=model.decision_lvl;
    vi->model_height=kv_size(model.model_stack)-1;
    vi->last_phase=lit_is_positive(lit);
}

inline void set_true_due_to_decision(Literal lit, Model model){
    dassert(is_undef(lit,model));
    model.decision_lvl++;
    model.last2propagated++;
    model.lastNpropagated++;
    model.last3propagated++;
    push(model.dlMarker,model);
    set_true_due_to_reason(lit,no_reason(),model);
}

inline bool lit_is_of_current_DL(Literal lit, Model model){
    return kv_A(model.vinfo,var(lit)).decision_lvl==model.decision_lvl;
}

inline unsigned int lit_DL(Literal lit, Model model){
    return kv_A(model.vinfo,var(lit)).decision_lvl;
}

inline unsigned int lit_height(Literal lit, Model model){
    return kv_A(model.vinfo,var(lit)).model_height;
}

inline bool tPropagated(Literal lit, Model model){
    return (int)(kv_A(model.vinfo,var(lit)).model_height)<=model.last3propagated;
}

inline Literal pop_and_set_undef(Model model){
    Literal lit;
    lit=kv_pop(model.model_stack);
    if(lit==model.dlMarker){
        model.decision_lvl--;
        model.last2propagated=kv_size(model.model_stack)-1;
        model.lastNpropagated=kv_size(model.model_stack)-1;
        model.last3propagated=kv_size(model.model_stack)-1;
        return zero_lit();
    }
    set_undef_in_assignment(lit,model);
    return(lit);
}

inline Literal next_lit_for_2Prop(Model model){
    if(model.last2propagated==kv_size(model.model_stack)-1) return(zero_lit());
    model.last2propagated++;
    return(kv_A(model.model_stack,model.last2propagated));
}

inline Literal next_lit_for_NProp(Model model){
    if(model.lastNpropagated==kv_size(model.model_stack)-1) return(zero_lit());
    model.lastNpropagated++;
    return(kv_A(model.model_stack,model.lastNpropagated));
}

inline Literal next_lit_for_TProp(Model model){
    if(model.last3propagated==kv_size(model.model_stack)-1) return(zero_lit());
    model.last3propagated++;
    return(kv_A(model.model_stack,model.last3propagated));
}

inline void set_last_TPropagated(unsigned int num_unused_lits, Model model){
    model.last3propagated=num_unused_lits-1;
}

inline bool last_phase(Var v, Model model){
    return kv_A(model.vinfo,v).last_phase;
}

inline void set_last_phase(Var v, bool phase, Model model){
    kv_A(model.vinfo,v).last_phase=phase;
}

inline Reason reason_of_lit(Literal lit, Model model){
    return kv_A(model.vinfo,var(lit)).r;
}

inline bool is_decision(Literal lit, Model model){
    return reason_of_lit(lit,model).is_unit_clause() && lit_DL(lit,model) > 0;
}

inline unsigned int model_size(Model model){
    return(kv_size(model.model_stack)-1-model.decision_lvl);
}
