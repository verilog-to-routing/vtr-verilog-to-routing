#ifndef _ccadical_h_INCLUDED
#define _ccadical_h_INCLUDED

#include "global.h"

/*------------------------------------------------------------------------*/
ABC_NAMESPACE_HEADER_START
/*------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdio.h>

// C wrapper for CaDiCaL's C++ API following IPASIR.

typedef struct CCaDiCaL CCaDiCaL;

const char *ccadical_signature (void);
CCaDiCaL *ccadical_init (void);
void ccadical_release (CCaDiCaL *);

void ccadical_add (CCaDiCaL *, int lit);
void ccadical_assume (CCaDiCaL *, int lit);
int ccadical_solve (CCaDiCaL *);
int ccadical_val (CCaDiCaL *, int lit);
int ccadical_failed (CCaDiCaL *, int lit);

void ccadical_set_terminate (CCaDiCaL *, void *state,
                             int (*terminate) (void *state));

void ccadical_set_learn (CCaDiCaL *, void *state, int max_length,
                         void (*learn) (void *state, int *clause));

/*------------------------------------------------------------------------*/

// Non-IPASIR conformant 'C' functions.

void ccadical_constrain (CCaDiCaL *, int lit);
int ccadical_constraint_failed (CCaDiCaL *);
void ccadical_set_option (CCaDiCaL *, const char *name, int val);
void ccadical_limit (CCaDiCaL *, const char *name, int limit);
int ccadical_get_option (CCaDiCaL *, const char *name);
void ccadical_print_statistics (CCaDiCaL *);
int64_t ccadical_active (CCaDiCaL *);
int64_t ccadical_irredundant (CCaDiCaL *);
int ccadical_fixed (CCaDiCaL *, int lit);
int ccadical_trace_proof (CCaDiCaL *, FILE *, const char *);
void ccadical_close_proof (CCaDiCaL *);
void ccadical_conclude (CCaDiCaL *);
void ccadical_terminate (CCaDiCaL *);
void ccadical_freeze (CCaDiCaL *, int lit);
int ccadical_frozen (CCaDiCaL *, int lit);
void ccadical_melt (CCaDiCaL *, int lit);
int ccadical_simplify (CCaDiCaL *);
int ccadical_vars (CCaDiCaL *);
int ccadical_reserve_difference (CCaDiCaL *, int number_of_vars);

// Extra

void ccadical_reserve(CCaDiCaL *, int min_max_var);
int ccadical_is_inconsistent(CCaDiCaL *);

/*------------------------------------------------------------------------*/

// Support legacy names used before moving to more IPASIR conforming names.

#define ccadical_reset ccadical_release
#define ccadical_sat ccadical_solve
#define ccadical_deref ccadical_val

/*------------------------------------------------------------------------*/
ABC_NAMESPACE_HEADER_END
/*------------------------------------------------------------------------*/

#endif
