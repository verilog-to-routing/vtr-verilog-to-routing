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
    module: gasp.c

    The "last_gasp" heuristic computes the reduction of each cube in
    the cover (without replacement) and then performs an expansion of
    these cubes.  The cubes which expand to cover some other cube are
    added to the original cover and irredundant finds a minimal subset.

    If one of the reduced cubes expands to cover some other reduced
    cube, then the new prime thus generated is a candidate for reducing
    the size of the cover.

    super_gasp is a variation on this strategy which extracts a minimal
    subset from the set of all prime implicants which cover all
    maximally reduced cubes.
*/

#include "espresso.h"

ABC_NAMESPACE_IMPL_START



/*
 *  reduce_gasp -- compute the maximal reduction of each cube of F
 *
 *  If a cube does not reduce, it remains prime; otherwise, it is marked
 *  as nonprime.   If the cube is redundant (should NEVER happen here) we
 *  just crap out ...
 *
 *  A cover with all of the cubes of F is returned.  Those that did
 *  reduce are marked "NONPRIME"; those that reduced are marked "PRIME".
 *  The cubes are in the same order as in F.
 */
static pcover reduce_gasp(F, D)
pcover F, D;
{
    pcube p, last, cunder, *FD;
    pcover G;

    G = new_cover(F->count);
    FD = cube2list(F, D);

    /* Reduce cubes of F without replacement */
    foreach_set(F, last, p) {
	cunder = reduce_cube(FD, p);
	if (setp_empty(cunder)) {
	    fatal("empty reduction in reduce_gasp, shouldn't happen");
	} else if (setp_equal(cunder, p)) {
	    SET(cunder, PRIME);			/* just to make sure */
	    G = sf_addset(G, p);		/* it did not reduce ... */
	} else {
	    RESET(cunder, PRIME);		/* it reduced ... */
	    G = sf_addset(G, cunder);
	}
	if (debug & GASP) {
	    printf("REDUCE_GASP: %s reduced to %s\n", pc1(p), pc2(cunder));
	}
	free_cube(cunder);
    }

    free_cubelist(FD);
    return G;
}

/*
 *  expand_gasp -- expand each nonprime cube of F into a prime implicant
 *
 *  The gasp strategy differs in that only those cubes which expand to
 *  cover some other cube are saved; also, all cubes are expanded
 *  regardless of whether they become covered or not.
 */

pcover expand_gasp(F, D, R, Foriginal)
INOUT pcover F;
IN pcover D;
IN pcover R;
IN pcover Foriginal;
{
    int c1index;
    pcover G;

    /* Try to expand each nonprime and noncovered cube */
    G = new_cover(10);
    for(c1index = 0; c1index < F->count; c1index++) {
	expand1_gasp(F, D, R, Foriginal, c1index, &G);
    }
    G = sf_dupl(G);
    G = expand(G, R, /*nonsparse*/ FALSE);	/* Make them prime ! */
    return G;
}



/*
 *  expand1 -- Expand a single cube against the OFF-set, using the gasp strategy
 */
void expand1_gasp(F, D, R, Foriginal, c1index, G)
pcover F;		/* reduced cubes of ON-set */
pcover D;		/* DC-set */
pcover R;		/* OFF-set */
pcover Foriginal;	/* ON-set before reduction (same order as F) */
int c1index;		/* which index of F (or Freduced) to be checked */
pcover *G;
{
    register int c2index;
    register pcube p, last, c2under;
    pcube RAISE, FREESET, temp, *FD, c2essential;
    pcover F1;

    if (debug & EXPAND1) {
	printf("\nEXPAND1_GASP:    \t%s\n", pc1(GETSET(F, c1index)));
    }

    RAISE = new_cube();
    FREESET = new_cube();
    temp = new_cube();

    /* Initialize the OFF-set */
    R->active_count = R->count;
    foreach_set(R, last, p) {
	SET(p, ACTIVE);
    }
    /* Initialize the reduced ON-set, all nonprime cubes become active */
    F->active_count = F->count;
    foreachi_set(F, c2index, c2under) {
	if (c1index == c2index || TESTP(c2under, PRIME)) {
	    F->active_count--;
	    RESET(c2under, ACTIVE);
	} else {
	    SET(c2under, ACTIVE);
	}
    }

    /* Initialize the raising and unassigned sets */
    (void) set_copy(RAISE, GETSET(F, c1index));
    (void) set_diff(FREESET, cube.fullset, RAISE);

    /* Determine parts which must be lowered */
    essen_parts(R, F, RAISE, FREESET);

    /* Determine parts which can always be raised */
    essen_raising(R, RAISE, FREESET);

    /* See which, if any, of the reduced cubes we can cover */
    foreachi_set(F, c2index, c2under) {
	if (TESTP(c2under, ACTIVE)) {
	    /* See if this cube can be covered by an expansion */
	    if (setp_implies(c2under, RAISE) ||
	      feasibly_covered(R, c2under, RAISE, temp)) {
		
		/* See if c1under can expanded to cover c2 reduced against
		 * (F - c1) u c1under; if so, c2 can definitely be removed !
		 */

		/* Copy F and replace c1 with c1under */
		F1 = sf_save(Foriginal);
		(void) set_copy(GETSET(F1, c1index), GETSET(F, c1index));

		/* Reduce c2 against ((F - c1) u c1under) */
		FD = cube2list(F1, D);
		c2essential = reduce_cube(FD, GETSET(F1, c2index));
		free_cubelist(FD);
		sf_free(F1);

		/* See if c2essential is covered by an expansion of c1under */
		if (feasibly_covered(R, c2essential, RAISE, temp)) {
		    (void) set_or(temp, RAISE, c2essential);
		    RESET(temp, PRIME);		/* cube not prime */
		    *G = sf_addset(*G, temp);
		}
		set_free(c2essential);
	    }
	}
    }

    free_cube(RAISE);
    free_cube(FREESET);
    free_cube(temp);
}

/* irred_gasp -- Add new primes to F and find an irredundant subset */
pcover irred_gasp(F, D, G)
pcover F, D, G;                 /* G is disposed of */
{
    if (G->count != 0)
	F = irredundant(sf_append(F, G), D);
    else
	free_cover(G);
    return F;
}


/* last_gasp */
pcover last_gasp(F, D, R, cost)
pcover F, D, R;
cost_t *cost;
{
    pcover G, G1;

    EXECUTE(G = reduce_gasp(F, D), GREDUCE_TIME, G, *cost);
    EXECUTE(G1 = expand_gasp(G, D, R, F), GEXPAND_TIME, G1, *cost);
    free_cover(G);
    EXECUTE(F = irred_gasp(F, D, G1), GIRRED_TIME, F, *cost);
    return F;
}


/* super_gasp */
pcover super_gasp(F, D, R, cost)
pcover F, D, R;
cost_t *cost;
{
    pcover G, G1;

    EXECUTE(G = reduce_gasp(F, D), GREDUCE_TIME, G, *cost);
    EXECUTE(G1 = all_primes(G, R), GEXPAND_TIME, G1, *cost);
    free_cover(G);
    EXEC(G = sf_dupl(sf_append(F, G1)), "NEWPRIMES", G);
    EXECUTE(F = irredundant(G, D), IRRED_TIME, F, *cost);
    return F;
}
ABC_NAMESPACE_IMPL_END

