/****************************************************************************************[solver.h]
Copyright (c) 2008, Niklas Sorensson
              2008, Koen Claessen

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef ABC__sat__lsat__solver_h
#define ABC__sat__lsat__solver_h


ABC_NAMESPACE_HEADER_START


// SolverTypes:
//
typedef struct solver_t solver;
typedef int solver_Var;
typedef int solver_Lit;
typedef int solver_lbool;

// Constants: (can these be made inline-able?)
//

extern const solver_lbool solver_l_True;
extern const solver_lbool solver_l_False;
extern const solver_lbool solver_l_Undef;


solver*      solver_new             (void);
void         solver_delete          (solver* s);
             
solver_Var   solver_newVar          (solver *s);
solver_Lit   solver_newLit          (solver *s);
             
solver_Lit   solver_mkLit           (solver_Var x);
solver_Lit   solver_mkLit_args      (solver_Var x, int sign);
solver_Lit   solver_negate          (solver_Lit p);
                                    
solver_Var   solver_var             (solver_Lit p);
int          solver_sign            (solver_Lit p);
             
int          solver_addClause       (solver *s, int len, solver_Lit *ps);
void         solver_addClause_begin (solver *s);
void         solver_addClause_addLit(solver *s, solver_Lit p);
int          solver_addClause_commit(solver *s);
             
int          solver_simplify        (solver *s);
             
int          solver_solve           (solver *s, int len, solver_Lit *ps);
void         solver_solve_begin     (solver *s);
void         solver_solve_addLit    (solver *s, solver_Lit p);
int          solver_solve_commit    (solver *s);
             
int          solver_okay            (solver *s);
             
void         solver_setPolarity     (solver *s, solver_Var v, int b);
void         solver_setDecisionVar  (solver *s, solver_Var v, int b);

solver_lbool solver_get_l_True      (void);
solver_lbool solver_get_l_False     (void);
solver_lbool solver_get_l_Undef     (void);

solver_lbool solver_value_Var       (solver *s, solver_Var x);
solver_lbool solver_value_Lit       (solver *s, solver_Lit p);

solver_lbool solver_modelValue_Var  (solver *s, solver_Var x);
solver_lbool solver_modelValue_Lit  (solver *s, solver_Lit p);

int          solver_num_assigns     (solver *s);
int          solver_num_clauses     (solver *s);     
int          solver_num_learnts     (solver *s);     
int          solver_num_vars        (solver *s);  
int          solver_num_freeVars    (solver *s);

int          solver_conflict_len    (solver *s);
solver_Lit   solver_conflict_nthLit (solver *s, int i);

// Setters:

void         solver_set_verbosity   (solver *s, int v);

// Getters:

int          solver_num_conflicts   (solver *s);

/* TODO

    // Mode of operation:
    //
    int       verbosity;
    double    var_decay;
    double    clause_decay;
    double    random_var_freq;
    double    random_seed;
    double    restart_luby_start; // The factor with which the values of the luby sequence is multiplied to get the restart    (default 100)
    double    restart_luby_inc;   // The constant that the luby sequence uses powers of                                        (default 2)
    int       expensive_ccmin;    // FIXME: describe.
    int       rnd_pol;            // FIXME: describe.

    int       restart_first;      // The initial restart limit.                                                                (default 100)
    double    restart_inc;        // The factor with which the restart limit is multiplied in each restart.                    (default 1.5)
    double    learntsize_factor;  // The intitial limit for learnt clauses is a factor of the original clauses.                (default 1 / 3)
    double    learntsize_inc;     // The limit for learnt clauses is multiplied with this factor each restart.                 (default 1.1)

    int       learntsize_adjust_start_confl;
    double    learntsize_adjust_inc;

    // Statistics: (read-only member variable)
    //
    uint64_t starts, decisions, rnd_decisions, propagations, conflicts;
    uint64_t dec_vars, clauses_literals, learnts_literals, max_literals, tot_literals;
*/



ABC_NAMESPACE_HEADER_END

#endif
