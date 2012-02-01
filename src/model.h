#ifndef _MODEL_H_
#define _MODEL_H_

#include "kvec.h" /*TO DO*/
#include "varinfo.h"
#include "heap.h"
#include "common.h"
#include "varMarks.h"

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
void init_in_assignment(Var var, Model model);
Model init_model(unsigned int num_vars);

inline bool lit_is_of_current_DL(Literal lit, Model model);
inline unsigned int lit_DL(Literal lit, Model model);
inline unsigned int lit_height(Literal lit, Model model);
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
inline bool tPropagated(Literal lit, Model model);
inline void print(Model model);
inline Literal get_next_marked_literal(VarMarks var_marks, Model model);
inline unsigned int model_size(Model model);

#endif /* _MODEL_H_ */