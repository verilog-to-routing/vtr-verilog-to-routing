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
    module: essen.c
    purpose: Find essential primes in a multiple-valued function
*/

#include "espresso.h"

ABC_NAMESPACE_IMPL_START


/*
    essential -- return a cover consisting of the cubes of F which are
    essential prime implicants (with respect to F u D); Further, remove
    these cubes from the ON-set F, and add them to the OFF-set D.

    Sometimes EXPAND can determine that a cube is not an essential prime.
    If so, it will set the "NONESSEN" flag in the cube.

    We count on IRREDUNDANT to have set the flag RELESSEN to indicate
    that a prime was relatively essential (i.e., covers some minterm
    not contained in any other prime in the current cover), or to have
    reset the flag to indicate that a prime was relatively redundant
    (i.e., all minterms covered by other primes in the current cover).
    Of course, after executing irredundant, all of the primes in the
    cover are relatively essential, but we can mark the primes which
    were redundant at the start of irredundant and avoid an extra check
    on these primes for essentiality.
*/

pcover essential(Fp, Dp)
IN pcover *Fp, *Dp;
{
    register pcube last, p;
    pcover E, F = *Fp, D = *Dp;

    /* set all cubes in F active */
    (void) sf_active(F);

    /* Might as well start out with some cubes in E */
    E = new_cover(10);

    foreach_set(F, last, p) {
    /* don't test a prime which EXPAND says is nonessential */
    if (! TESTP(p, NONESSEN)) {
        /* only test a prime which was relatively essential */
        if (TESTP(p, RELESSEN)) {
        /* Check essentiality */
        if (essen_cube(F, D, p)) {
            if (debug & ESSEN)
            printf("ESSENTIAL: %s\n", pc1(p));
            E = sf_addset(E, p);
            RESET(p, ACTIVE);
            F->active_count--;
        }
        }
    }
    }

    *Fp = sf_inactive(F);               /* delete the inactive cubes from F */
    *Dp = sf_join(D, E);        /* add the essentials to D */
    sf_free(D);
    return E;
}

/*
    essen_cube -- check if a single cube is essential or not

    The prime c is essential iff

    consensus((F u D) # c, c) u D

    does not contain c.
*/
bool essen_cube(F, D, c)
IN pcover F, D;
IN pcube c;
{
    pcover H, FD;
    pcube *H1;
    bool essen;

    /* Append F and D together, and take the sharp-consensus with c */
    FD = sf_join(F, D);
    H = cb_consensus(FD, c);
    free_cover(FD);

    /* Add the don't care set, and see if this covers c */
    H1 = cube2list(H, D);
    essen = ! cube_is_covered(H1, c);
    free_cubelist(H1);

    free_cover(H);
    return essen;
}


/*
 *  cb_consensus -- compute consensus(T # c, c)
 */
pcover cb_consensus(T, c)
register pcover T;
register pcube c;
{
    register pcube temp, last, p;
    register pcover R;

    R = new_cover(T->count*2);
    temp = new_cube();
    foreach_set(T, last, p) {
    if (p != c) {
        switch (cdist01(p, c)) {
        case 0:
            /* distance-0 needs special care */
            R = cb_consensus_dist0(R, p, c);
            break;

        case 1:
            /* distance-1 is easy because no sharping required */
            consensus(temp, p, c);
            R = sf_addset(R, temp);
            break;
        }
    }
    }
    set_free(temp);
    return R;
}


/*
 *  form the sharp-consensus for p and c when they intersect
 *  What we are forming is consensus(p # c, c).
 */
pcover cb_consensus_dist0(R, p, c)
pcover R;
register pcube p, c;
{
    int var;
    bool got_one;
    register pcube temp, mask;
    register pcube p_diff_c=cube.temp[0], p_and_c=cube.temp[1];

    /* If c contains p, then this gives us no information for essential test */
    if (setp_implies(p, c)) {
    return R;
    }

    /* For the multiple-valued variables */
    temp = new_cube();
    got_one = FALSE;
    INLINEset_diff(p_diff_c, p, c);
    INLINEset_and(p_and_c, p, c);

    for(var = cube.num_binary_vars; var < cube.num_vars; var++) {
    /* Check if c(var) is contained in p(var) -- if so, no news */
    mask = cube.var_mask[var];
    if (! setp_disjoint(p_diff_c, mask)) {
        INLINEset_merge(temp, c, p_and_c, mask);
        R = sf_addset(R, temp);
        got_one = TRUE;
    }
    }

    /* if no cube so far, add one for the intersection */
    if (! got_one && cube.num_binary_vars > 0) {
    /* Add a single cube for the intersection of p and c */
    INLINEset_and(temp, p, c);
    R = sf_addset(R, temp);
    }

    set_free(temp);
    return R;
}
ABC_NAMESPACE_IMPL_END

