//===--- act_var.h ----------------------------------------------------------===
//
//                     satoko: Satisfiability solver
//
// This file is distributed under the BSD 2-Clause License.
// See LICENSE for details.
//
//===------------------------------------------------------------------------===
#ifndef satoko__act_var_h
#define satoko__act_var_h

#include "solver.h"
#include "types.h"
#include "utils/heap.h"
#include "utils/sdbl.h"

#include "misc/util/abc_global.h"
ABC_NAMESPACE_HEADER_START

/** Re-scale the activity value for all variables.
 */
static inline void var_act_rescale(solver_t *s)
{
    unsigned i;
    act_t *activity = vec_act_data(s->activity);

    for (i = 0; i < vec_sdbl_size(s->activity); i++)
        activity[i] = sdbl_div(activity[i], s->opts.var_act_rescale);
    s->var_act_inc = sdbl_div(s->var_act_inc, s->opts.var_act_rescale);
}

/** Increment the activity value of one variable ('var')
 */
static inline void var_act_bump(solver_t *s, unsigned var)
{
    act_t *activity = vec_act_data(s->activity);

    activity[var] = sdbl_add(activity[var], s->var_act_inc);
    if (activity[var] > s->opts.var_act_limit)
        var_act_rescale(s);
    if (heap_in_heap(s->var_order, var))
        heap_decrease(s->var_order, var);
}

/** Increment the value by which variables activity values are incremented
 */
static inline void var_act_decay(solver_t *s)
{
    s->var_act_inc = sdbl_mult(s->var_act_inc, double2sdbl(1 /s->opts.var_decay));
}

ABC_NAMESPACE_HEADER_END
#endif /* satoko__act_var_h */
