/*
 * Revision Control Information
 *
 * $Source$
 * $Author$
 * $Revision$
 * $Date$
 *
 */
/*
 *  Module: espresso.c
 *  Purpose: The main espresso algorithm
 *
 *  Returns a minimized version of the ON-set of a function
 *
 *  The following global variables affect the operation of Espresso:
 *
 *  MISCELLANEOUS:
 *      trace
 *          print trace information as the minimization progresses
 *
 *      remove_essential
 *          remove essential primes
 *
 *      single_expand
 *          if true, stop after first expand/irredundant
 *
 *  LAST_GASP or SUPER_GASP strategy:
 *      use_super_gasp
 *          uses the super_gasp strategy rather than last_gasp
 *
 *  SETUP strategy:
 *      recompute_onset
 *          recompute onset using the complement before starting
 *
 *      unwrap_onset
 *          unwrap the function output part before first expand
 *
 *  MAKE_SPARSE strategy:
 *      force_irredundant
 *          iterates make_sparse to force a minimal solution (used
 *          indirectly by make_sparse)
 *
 *      skip_make_sparse
 *          skip the make_sparse step (used by opo only)
 */

#include "espresso.h"

ABC_NAMESPACE_IMPL_START


pcover espresso(F, D1, R)
pcover F, D1, R;
{
    pcover E, D, Fsave;
    pset last, p;
    cost_t cost, best_cost;

begin:
    Fsave = sf_save(F);        /* save original function */
    D = sf_save(D1);        /* make a scratch copy of D */

    /* Setup has always been a problem */
    if (recompute_onset) {
    EXEC(E = simplify(cube1list(F)),     "SIMPLIFY   ", E);
    free_cover(F);
    F = E;
    }
    cover_cost(F, &cost);
    if (unwrap_onset && (cube.part_size[cube.num_vars - 1] > 1)
      && (cost.out != cost.cubes*cube.part_size[cube.num_vars-1])
      && (cost.out < 5000))
    EXEC(F = sf_contain(unravel(F, cube.num_vars - 1)), "SETUP      ", F);

    /* Initial expand and irredundant */
    foreach_set(F, last, p) {
    RESET(p, PRIME);
    }
    EXECUTE(F = expand(F, R, FALSE), EXPAND_TIME, F, cost);
    EXECUTE(F = irredundant(F, D), IRRED_TIME, F, cost);

    if (! single_expand) {
    if (remove_essential) {
        EXECUTE(E = essential(&F, &D), ESSEN_TIME, E, cost);
    } else {
        E = new_cover(0);
    }

    cover_cost(F, &cost);
    do {

        /* Repeat inner loop until solution becomes "stable" */
        do {
        copy_cost(&cost, &best_cost);
        EXECUTE(F = reduce(F, D), REDUCE_TIME, F, cost);
        EXECUTE(F = expand(F, R, FALSE), EXPAND_TIME, F, cost);
        EXECUTE(F = irredundant(F, D), IRRED_TIME, F, cost);
        } while (cost.cubes < best_cost.cubes);

        /* Perturb solution to see if we can continue to iterate */
        copy_cost(&cost, &best_cost);
        if (use_super_gasp) {
        F = super_gasp(F, D, R, &cost);
        if (cost.cubes >= best_cost.cubes)
            break;
        } else {
        F = last_gasp(F, D, R, &cost);
        }

    } while (cost.cubes < best_cost.cubes ||
        (cost.cubes == best_cost.cubes && cost.total < best_cost.total));

    /* Append the essential cubes to F */
    F = sf_append(F, E);                /* disposes of E */
    if (trace) size_stamp(F, "ADJUST     ");
    }

    /* Free the D which we used */
    free_cover(D);

    /* Attempt to make the PLA matrix sparse */
    if (! skip_make_sparse) {
    F = make_sparse(F, D1, R);
    }

    /*
     *  Check to make sure function is actually smaller !!
     *  This can only happen because of the initial unravel.  If we fail,
     *  then run the whole thing again without the unravel.
     */
    if (Fsave->count < F->count) {
    free_cover(F);
    F = Fsave;
    unwrap_onset = FALSE;
    goto begin;
    } else {
    free_cover(Fsave);
    }

    return F;
}
ABC_NAMESPACE_IMPL_END

