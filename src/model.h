#ifndef _MODEL_H_
#define _MODEL_H_

#include "kvec.h" /*TO DO*/
#include "varinfo.h"
#include "heap.h"
#include "common.h"

typedef struct _model {
        kvec_t(Literal) model_stack; /* Keeps track of the state of the model */ 
        int last2propagated, last3propagated, lastNpropagated;
        kvec_t(char) assignment; /*truth value of each literal*/
        unsigned int decision_lvl;
        kvec_t(VarInfo) vinfo; /*keeps track of each variable*/
        MaxHeap maxheap;
        short unsigned int *vassignment;
        Literal dlMarker;
        unsigned int n_vars;
        unsigned int n_lits;
} Model;

void set_true_due_to_decision(Literal l, Model model);
void set_true_due_to_reason(Literal l, Reason r, Model model);

inline void push(Literal lit, Model model);
inline void set_true_in_assignment(Literal lit, Model model);
inline void set_undef_in_assignment(Literal lit, Model model);
void init_in_assignment(Var var);
Model init_model(unsigned int num_vars);

inline bool lit_is_of_current_DL(Literal lit, Model model);
inline unsigned int lit_DL(Literal lit, Model model);
inline unsigned int lit_height(Literal lit);
inline Literal  pop_and_set_undef(Model model);
inline bool is_true(Literal lit, Model model);
inline bool is_false(Literal lit, Model model);
inline bool is_undef(Literal lit, Model model);
inline bool is_true_or_undef(Literal lit, Model model);
inline bool is_undef_var(Var v, Model model);
inline Literal  next_lit_for_2Prop(Model model);
inline Literal  next_lit_for_NProp(Model model);
inline Literal  next_lit_for_TProp(Model model);
inline void set_last_TPropagated(unsigned int num_unused_lits, Model model);
inline bool last_phase(Var v, Model model);
inline void set_last_phase(Var v, bool phase, Model model);
inline Reason reason_of_lit (Literal lit, Model model);
inline bool is_decision (Literal lit, Model model);
inline void add_new_var (Model model );
inline bool TPropagated(Literal lit, Model model);
inline void print(Model model);
inline Literal get_next_marked_literal(VarMarks var_marks, Model model);
inline unsigned int size(Model model);

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
    kv_resize(Literal,model.vinfo,model.n_vars+1);
    model.assignment = model.n_lits;
    model.vassignment = ((short unsigned *)((char *)model.assignment));
    model.dlMarker = zero_lit();
    model.decision_lvl = 0;
    for(unsigned int v=0;v<=model.n_vars;v++){
        init_in_assignment(v);
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
      lit.print();
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
  } while(!var_marks.isMarked(var(lit)));
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

#endif /* _MODEL_H_ */