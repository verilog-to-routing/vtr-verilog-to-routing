//===--- satoko.h -----------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__satoko_h
#define satoko__satoko_h

#include "types.h"
#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

/** Return valeus */
enum {
    SATOKO_ERR = 0,
    SATOKO_OK  = 1
};

enum {
    SATOKO_UNDEC = 0, /* Undecided */
    SATOKO_SAT   = 1,
    SATOKO_UNSAT = -1
};

enum {
    SATOKO_LIT_FALSE = 1,
    SATOKO_LIT_TRUE = 0,
    SATOKO_VAR_UNASSING = 3
};

struct solver_t_;
typedef struct solver_t_ satoko_t;

typedef struct satoko_opts satoko_opts_t;
struct satoko_opts {
    /* Limits */
    long conf_limit;  /* Limit on the n.of conflicts */
    long prop_limit;  /* Limit on the n.of propagations */

    /* Constants used for restart heuristic */
    double f_rst;          /* Used to force a restart */
    double b_rst;          /* Used to block a restart */
    unsigned fst_block_rst;   /* Lower bound n.of conflicts for start blocking restarts */
    unsigned sz_lbd_bqueue;   /* Size of the moving avarege queue for LBD (force restart) */
    unsigned sz_trail_bqueue; /* Size of the moving avarege queue for Trail size (block restart) */

    /* Constants used for clause database reduction heuristic */
    unsigned n_conf_fst_reduce;  /* N.of conflicts before first clause databese reduction */
    unsigned inc_reduce;         /* Increment to reduce */
    unsigned inc_special_reduce; /* Special increment to reduce */
    unsigned lbd_freeze_clause;
    float learnt_ratio;          /* Percentage of learned clauses to remove */

    /* VSIDS heuristic */
    double var_decay;
    float clause_decay;
    unsigned var_act_rescale;
    act_t var_act_limit;


    /* Binary resolution */
    unsigned clause_max_sz_bin_resol;
    unsigned clause_min_lbd_bin_resol;
    float garbage_max_ratio;
    char verbose;
    char no_simplify;
};

typedef struct satoko_stats satoko_stats_t;
struct satoko_stats {
    unsigned n_starts;
    unsigned n_reduce_db;

    long n_decisions;
    long n_propagations;
    long n_propagations_all;
    long n_inspects;
    long n_conflicts;
    long n_conflicts_all;

    long n_original_lits;
    long n_learnt_lits;
};


//===------------------------------------------------------------------------===
extern satoko_t *satoko_create(void);
extern void satoko_destroy(satoko_t *);
extern void satoko_reset(satoko_t *);

extern void satoko_default_opts(satoko_opts_t *);
extern void satoko_configure(satoko_t *, satoko_opts_t *);
extern int  satoko_parse_dimacs(char *, satoko_t **);
extern void satoko_setnvars(satoko_t *, int);
extern int  satoko_add_variable(satoko_t *, char);
extern int  satoko_add_clause(satoko_t *, int *, int);
extern void satoko_assump_push(satoko_t *s, int);
extern void satoko_assump_pop(satoko_t *s);
extern int  satoko_simplify(satoko_t *);
extern int  satoko_solve(satoko_t *);
extern int  satoko_solve_assumptions(satoko_t *s, int * plits, int nlits);
extern int  satoko_solve_assumptions_limit(satoko_t *s, int * plits, int nlits, int nconflim);
extern int  satoko_minimize_assumptions(satoko_t *s, int * plits, int nlits, int nconflim);
extern void satoko_mark_cone(satoko_t *, int *, int);
extern void satoko_unmark_cone(satoko_t *, int *, int);

extern void satoko_rollback(satoko_t *);
extern void satoko_bookmark(satoko_t *);
extern void satoko_unbookmark(satoko_t *);
/* If problem is unsatisfiable under assumptions, this function is used to
 * obtain the final conflict clause expressed in the assumptions.
 *  - It receives as inputs the solver and a pointer to an array where clause
 * is stored. The memory for the clause is managed by the solver.
 *  - The return value is either the size of the array or -1 in case the final
 * conflict cluase was not generated.
 */
extern int satoko_final_conflict(satoko_t *, int **);

/* Procedure to dump a DIMACS file.
 * - It receives as input the solver, a file name string and two integers.
 * - If the file name string is NULL the solver will dump in stdout.
 * - The first argument can be either 0 or 1 and is an option to dump learnt 
 *   clauses. (value 1 will dump them).
 * - The seccond argument can be either 0 or 1 and is an option to use 0 as a 
 *   variable ID or not. Keep in mind that if 0 is used as an ID the generated
 *   file will not be a DIMACS. (value 1 will use 0 as ID).
 */
extern void satoko_write_dimacs(satoko_t *, char *, int, int);
extern satoko_stats_t * satoko_stats(satoko_t *);
extern satoko_opts_t * satoko_options(satoko_t *);

extern int satoko_varnum(satoko_t *);
extern int satoko_clausenum(satoko_t *);
extern int satoko_learntnum(satoko_t *);
extern int satoko_conflictnum(satoko_t *);
extern void satoko_set_stop(satoko_t *, int *);
extern void satoko_set_stop_func(satoko_t *s, int (*fnct)(int));
extern void satoko_set_runid(satoko_t *, int);
extern int satoko_read_cex_varvalue(satoko_t *, int);
extern abctime satoko_set_runtime_limit(satoko_t *, abctime);
extern char satoko_var_polarity(satoko_t *, unsigned);


ABC_NAMESPACE_HEADER_END
#endif /* satoko__satoko_h */
