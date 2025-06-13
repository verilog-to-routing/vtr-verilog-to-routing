#ifndef _decide_h_INCLUDED
#define _decide_h_INCLUDED

#include "global.h"
ABC_NAMESPACE_HEADER_START

struct kissat;

void kissat_decide (struct kissat *);
void kissat_start_random_sequence (struct kissat *);
void kissat_internal_assume (struct kissat *, unsigned lit);
unsigned kissat_next_decision_variable (struct kissat *);
int kissat_decide_phase (struct kissat *, unsigned idx);

#define INITIAL_PHASE (GET_OPTION (phase) ? 1 : -1)

ABC_NAMESPACE_HEADER_END

#endif
